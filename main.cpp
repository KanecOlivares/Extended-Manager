#include <vector>
#include <array>
#include <sstream>
#include <algorithm>
#include <format> // Require CPP +20
#include <string>
#include <fstream>
#include <filesystem>
#include <iostream>
#include "process.h"
using namespace std;

bool detected_bug = false;

string GREEN = "\033[32m";
// string NORMAL = "\033[0m"; using normal from proccess.h
// string RED = "\033[31m"; using red from process.h

vector<Process> PCB;
int max_pcb = 16;
vector<Resource> RCB;

// Ready and Waitl ist
// They only store the pid of their respective processes
vector<vector<int>> RL; //pid
vector<int> WL;

// Declarations for compiler
int highest_occupied_priority();
Process* get_process(int pid);
void delete_process(int target_pid);
void release(int release_rid, int given_pid, int units);
void scheduler();

int next_pid = 1;

void debug(string msg){
    cout << RED << "ERROR: ";
    cout << msg << NORMAL <<  endl;
    detected_bug = true;
}

Process* get_running_process(){
    /*
    Return the Pointer to running process
    */

    if (RL.empty()){
        debug("Ready list is empty.");
        return nullptr;
    }

    // for (size_t i = RL.size(); i >= 0; --i){
    //     if (RL[i].empty()){
    //         continue;
    //     }else{
    //         return get_process(RL[i][0]);
    //     }
    // }
    return get_process(RL[highest_occupied_priority()][0]);
}

void after_exe(){
    ofstream file("output.txt", ios::app);
    if (!file.is_open()) {
        cerr << "Error opening file!" << endl;
        return;
    }

    Process* running_p = get_running_process();
    if (!running_p) {
        cerr << "No running process found!" << endl;
        return;
    }

    int run_pid = running_p->pid;
    file << run_pid << " ";  // appends instead of overwriting
}

void print(string msg){
    cout << msg << endl;
}

bool is_proccess_0(int pid){
    return pid == 0;
}

Process* get_process(int pid){
    /*
    Returns a Process pointer if found to have the same PID
    Returns nullptr if not found
    */

    for (Process& c : PCB){
        if (c.pid == pid){
            return &c;
        }
    }
    string msg = format("Did not find process with PID: {}", pid);
    debug(msg);
    return nullptr;
}



void add_child_to(int parent_pid, int child_pid){
    Process* parent = get_process(parent_pid);
    Process* child = get_process(child_pid);
    /*
    Adding the process with child_pid to the list of childrent of process with parent_pid
    */
    
    if (!parent || !child){
        debug("Child or Parent are null so cannot add child to parent list");
        return;
    }

    parent -> add_child(child -> pid);
}

bool pcb_full(){
    return PCB.size() == 16;
}

void create(int priority){
    /*
    Running PID creates the child. Use next_pid and post increment for the next create()
    */

    if (priority <= 0){
        debug("Cannot access priority <= 0");
        return;
    }
    int size = RL.size();
    if (priority > size){
        debug("That prirotity level doesnt exist.");
        return;
    }

    if (pcb_full()){
        debug("PCB is full. Cannot Create more Proccesses");
        return;
    }

    Process* runnning_p = get_running_process();

    if (!runnning_p){
        debug("Nullptr can not create a process");
        return;
    }
    int run_pid = runnning_p -> pid;
    int run_priority = runnning_p -> priority;
    Process child = Process(next_pid++, 0, run_pid, priority); // pid, state (0 = ready), parent_pid
    PCB.push_back(child);

    Process* parent = get_running_process();
    Process* child_ptr = get_process(child.pid);

    add_child_to(parent -> pid, child_ptr -> pid);
    RL[priority].push_back(child_ptr -> pid);

    string msg = format("Process {} created", child.pid);
    print(msg);

    if (priority > run_priority){
        scheduler();
    }
}

void delete_children(int parent_pid){
    /*
    For all children in the parent's child vector delete those aswell
    */

    Process* parent = get_process(parent_pid);

    if (!parent){
        debug("Trying to delete children of nullptr");
        return;
    }

    for (int child_pid : parent -> children){
        delete_process(child_pid);
    }
    parent -> children.clear();

    
}

void remove_from_PCB(int target_pid){
    /*
    Removes the target process* in PCB
    Had to do remove_if b/c there was a mismatch.

    Compile error:
    std::remove<std::__wrap_iter<Process *>, Process>' requested here 155 | 
        PCB.erase(remove(PCB.begin(), PCB.end(), *target),

    Solution: Use remove_if and use the pids.
    */

    Process* target = get_process(target_pid);

    if (!target){
        debug("Attempting to remove nullptr from PCB");
        return;
    }
    if (is_proccess_0(target_pid)){
        debug("Attempting to remove Process 0 from PCB");
        return;
    }
    PCB.erase(remove_if(PCB.begin(), PCB.end(),
                         [target_pid](const Process& p){ return p.pid == target_pid; }),
          PCB.end());

}

void remove_from_parent_list(int child_pid){
    /*
    Given a child Process* it will obtain the parent process* and
    remove the child from parent.childlist
    */

    Process* child = get_process(child_pid);

    if (!child){
        debug("Attempting to remove from parent list. Child is nullptr ");
        return;
    }
    Process* parent = get_process(child->parent);
    if (!parent){
        debug("Parent is nullptr. Cannot remove child from nullptr");
        return;
    }

    parent->remove_child(child_pid);
}

void remove_from_lists(int target_pid){
    /*
    Removes the target from RL or WL. It is safe due to erase(remove()) does
    nothing if it does't exist in the waitlist
    */
    Process* target_ptr = get_process(target_pid);
    int priority = target_ptr -> priority;
    RL[priority].erase(remove(RL[priority].begin(), RL[priority].end(), target_pid), RL[priority].end());
    WL.erase(remove(WL.begin(), WL.end(), target_pid), WL.end());
}

void release_resources(int target_pid){
    /*
    Release all the resources of target
    */
    Process* target = get_process(target_pid);
    if (!target){
        debug("Cannot release resources of nullptr");
        return;
    }
    for (const auto &[rid, units] : target -> resources){
        release(rid, target_pid, units);
    }
}

void delete_process(int target_pid){
    /*
    Delete Children first then delete target
    */
   Process* target = get_process(target_pid);

    if (is_proccess_0(target_pid)){
        print("Skipping Deletion of Process 0");
        return;
    }

    if (!target){
        debug("Attempted to delete nullptr");
        return;
    }

    if (! target -> children.empty()){
        delete_children(target_pid);
    }

    target = get_process(target_pid);

    if(target -> parent > 0){
        remove_from_parent_list(target_pid); // removes pid from children vector
    }

    remove_from_lists(target_pid); // removes from RL or WL which ever they are in
    release_resources(target_pid);
    remove_from_PCB(target_pid);
    string msg = format("Destroyed process with pid {}", target_pid);
    print(msg);
}

void destroy(int target_pid){
    Process* running_p = get_running_process();
    
    if (!running_p){
        debug("Nullptr cannot destroy any processes");
        return;
    }
    int run_pid = running_p -> pid;

    if (is_proccess_0(target_pid)){
        debug("Process 0 cannot be destroyed during runtime");
        return;
    }
    if (running_p -> has_child(target_pid) || target_pid == running_p -> pid){
        delete_process(target_pid);
    }else{
        string msg = format("Running process {} can not destroy {} because it is not owner.", run_pid, target_pid);
        debug(msg);
        return;
    }
}

Resource* get_resource(int target_rid){
    int size = static_cast<int>(RCB.size());
    if (target_rid > size){
        debug("Trying to get resource that does not exist.");
        return nullptr;
    }
    return &RCB[target_rid];
}

void move_process_to_other(vector<int>& from, vector<int>& to, int target_pid){
    /*
    Erase the PID from the from list and move to the to list. 
    */
    from.erase(remove(from.begin(), from.end(), target_pid), from.end());
    to.push_back(target_pid);
}

void request(int request_rid, int units){
    /*
    Running process requesting resource with rid = request_rid
    */
    Process* runnning_p = get_running_process();
    int run_pid = runnning_p -> pid;
    int priority = runnning_p -> priority;
    if (!runnning_p){
        debug("Nullptr can not request a resource.");
        return;
    }

    Resource* rr = get_resource(request_rid);  // Requested Resource ptr
    if (!rr){
        debug("Requested resource does not exist.");
        return;
    }

    if(is_proccess_0(run_pid)){
        debug("Process 0 can not request any resources.");
        return;
    }

    if (rr -> is_request_legal(units)){
        rr -> alloc(units); // no need for PID just allocating
        runnning_p->add_resource(request_rid, units); 
        string msg = format("Resource with rid: {} has been allocated to process with pid {}", request_rid, runnning_p ->pid);
        print(msg);

    }else{ // Reousrce is not free or too many resources requested
        runnning_p = get_process(run_pid);
        runnning_p -> block();
        move_process_to_other(RL[priority], WL, run_pid); // Move from RL to WL the process being moved is running_p
        rr->add_wl_process(run_pid, units);
        string msg = format("Process with PID: {} is now bloacked", runnning_p -> pid);
        print(msg);
        scheduler();
    }
}

int get_highest_priority(vector<int>& v){
    int max_priority = -1;
    int max_pid = -1;
    for (int pid : v){
        Process* p = get_process(pid);
        int p_priority = p -> priority;
        if (p_priority > max_priority){
            max_pid = p -> pid;
        }
    }
    return max_pid;
}

void rl_free_pids(vector<int>& free_pids){
    for (int pid : free_pids){
        Process* p = get_process(pid);
        int priority = p -> priority;
        move_process_to_other(WL, RL[priority], pid);
    }
}

void rl_scheduler(vector<int>& free_pids){

    int next_ready_pid = get_highest_priority(free_pids); // get the highest priority in free_pids
    Process* runnning_p = get_running_process();
    int run_priority = runnning_p -> priority;
    Process* next_ready_process = get_process(next_ready_pid);
    int next_priority = next_ready_process -> priority;

    if (next_priority > run_priority){
        scheduler();
    }

}

void release(int release_rid, int given_pid, int units){
    /*
    Releases resource w/rid from given_p.
    units = untis to be released.
    */

    Process* given_p = get_process(given_pid);

    if(!given_p){
        debug("Nullptr can not release resource.");
        return;
    }

    Resource* rr = get_resource(release_rid); // release resource

    if (!rr){
        string msg = format("Can't release nullptr", release_rid);
        debug(msg);
        return;
    }

    if (! given_p-> has_resource(release_rid, units)){ // given program is not owner of resource
        string msg = format("Given Process PID: {} does not own resouce {} or amount of {}", given_p -> pid, release_rid, units);
        debug(msg);
        return;
    }


    rr -> release(units);
    given_p->remove_resource(release_rid, units);

    if (! rr -> WL.empty()){
        vector<int> free_pids = rr -> now_free();
        rl_free_pids(free_pids); // frees pids of free_pids
        rl_scheduler(free_pids); // Calls scheduler if needed
    }

    string msg = format("Resource RID: {}, units: {}, has been released", release_rid, units);
    print(msg);
    
}

int highest_occupied_priority(){

    for (int i = RL.size() - 1; i >= 0; --i){
        if (RL[i].empty()){
            continue;
        }else{
            return i;
        }
    }

    return -1;
}

void timeout(){
    /*
    Mimcks time sharing

    */
   int high_p = highest_occupied_priority();
    if (RL[high_p].size() == 1){
        print("Only 1 process in ready list. Just skip");
        return;
    }
    
    Process* running_p = get_running_process();
    rotate(RL[high_p].begin(), RL[high_p].begin() + 1, RL[high_p].end());
    running_p -> ready(); // Now it is the old running process is ready()
    Process* new_running_p = get_running_process();
    new_running_p -> running(); // New running_p is now running()
    scheduler();

}

void scheduler(){
    Process* running_p = get_running_process();

    if (!running_p){
        debug("Scheduler found nullptr as running.");
        return;
    }

    cout << "Process PID: " << running_p -> pid << " running." << endl;
}


void force_free_all(){
    for (auto &r : RCB){
        r.force_free();
    }
}

void pcb_clear_but_0(){
    if (PCB.size() > 1) {
        PCB.erase(PCB.begin() + 1, PCB.end());
    }
}

void rcb_default(){
    for (Resource& r : RCB){
        r.force_free();
    }
}

void rl_clear_but_0(){
    if (RL.size() > 1) {
        RL.erase(RL.begin() + 1, RL.end());
    }
    // RL.resize(3);
}

void make_resources(int num_resources){
    Resource r0 = Resource(0, 1);
    RCB.push_back(r0);
    for (int i = 1; i < num_resources; ++i){
        Resource r = Resource(i, i); // RID starts at 0, Size starts a 1
        RCB.push_back(r);
    }
}

void init(int levels, int num_resources){
    pcb_clear_but_0();
    rl_clear_but_0();

    RL.resize(levels);
    PCB[0].running();
    
    WL.clear();
    RCB.clear();
    make_resources(num_resources);
    // force_free_all(); // all resources

}

string prompt() {
    cout << ">> ";
    string input;
    std::getline(std::cin, input);
    return input;
}

vector<string> get_tokens(const std::string& input){
    istringstream iss(input);
    vector<string> tokens;
    string word;

    while (iss >> word) {  // splits on any whitespace
        tokens.push_back(word);
    }
    return tokens;
}

void see_values(){
    /*
    Check states: next_pid, WL, RL, RCB, PCB, running_p
    */

    // Running 
    Process* running_p = get_running_process();
    cout << GREEN << "RUNNING" << NORMAL << endl;
    running_p -> print();

    // PCB
    cout << GREEN << "PCB: " << NORMAL; 
    for (Process &p : PCB){
        p.print();
    }

    // RCB
    cout << GREEN << "RCB: " << NORMAL; 
    for (Resource &r : RCB){
        r.print();
    }
    cout << endl;

    // Ready List
    cout << GREEN << "RL: " << NORMAL;
    for (size_t i = 0; i < RL.size(); ++i){
        cout << "\t" << "Priority " << i << ": ";
        for(auto r : RL[i]){
            cout << r << " ";
        }
    }
    cout << endl;
    
    // for (int p : RL){
    //     cout << p << " ";
    // }
    // cout << endl;

    // Wait List
    cout << GREEN << "WL: " << NORMAL;
    for (int p : WL){
        cout << p << " ";
    }
    cout << endl;

}

void ru(vector<string> tokens){
    RCB.clear();
    int curr_rid = 0;
    for (auto token : tokens){
        int units = stoi(token);
        Resource r = Resource(curr_rid++, units); // RID starts at 0, Size starts a 1
        RCB.push_back(r);

    }
}

void id(){
    init(3,4);
    vector<string> tokens = {"1", "1", "2", "3"};
    ru(tokens);
}

bool take_action(vector<string>& tokens){
    string command = tokens[0];
    if (command == "cr"){
        // print("Made it to create()");
        int priority = stoi(tokens[1]);
        create(priority);
        scheduler();
        return true;
    }else if (command == "de"){
        int pid = stoi(tokens[1]);
        destroy(pid);
        return true;
    }else if(command == "rq"){
        int rid = stoi(tokens[1]);
        int units = stoi(tokens[2]);
        request(rid, units);
        return true;
    }else if(command == "rl"){
        int rid = stoi(tokens[1]);
        int units = stoi(tokens[2]);
        Process* p = get_running_process();
        int run_pid = p -> pid;
        release(rid, run_pid, units);
        return true;
    }else if(command == "to"){
        timeout();
        return true;
    }else if(command == "in"){
        int levels = stoi(tokens[1]);
        int num_r = stoi(tokens[2]);
        init(levels, num_r);
        return true;
    }else if(command == "see"){
        see_values();
        return true;
    }else if(command == "id"){
        id();
        return true;
    }
    print("Not a valid command");
    return false;

}

void clear_file(const string& filename) {
    if (filesystem::exists(filename)) {
        ofstream file(filename, std::ios::trunc);
    }
}

int main() {

    Process p0 = Process(0, 0, -1, 0); // PID 0, ready, no parent
    PCB.push_back(p0);
    RL.resize(1);
    RL[0].push_back(0);
    id();

    vector<std::string> tokens;
    clear_file("output.txt");
    

    for (;;) {
        string input = prompt();
        if (input == "q") break;

        tokens = get_tokens(input);
        if (tokens.empty()){
            debug("No tokens detected");
            continue;
        };

        if(take_action(tokens)){
            tokens.clear();
            after_exe();
            continue; // back to top
        }

        if (detected_bug) {
            for (;;) {
                input = prompt();
                tokens = get_tokens(input);
                if (!tokens.empty() && (tokens[0] == "in" || tokens[0] == "id")) {
                    break; // got a valid starter token
                }
            }
    
            // Now handle that command:
            if (take_action(tokens)) {
                tokens.clear();
                after_exe();
            }
        }

        // If action not taken, require a command starting with "in" or "id"
        
    }
}


// if (RL.size() > 1) {
//     RL.erase(RL.begin() + 1, RL.end());
// }
// RL.resize(size);
