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
string prompt();

int next_pid = 1;
int curr_running_pid = 0;

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

    return get_process(curr_running_pid);
}

void after_bug(){
    ofstream file("output.txt", ios::app);
    if (!file.is_open()) {
        cerr << "Error opening file!" << endl;
        return;
    }

    file << "-1 ";  // appends instead of overwriting
}

void before_restart(){
    ofstream file("output.txt", ios::app);
    if (!file.is_open()) {
        cerr << "Error opening file!" << endl;
        return;
    }

    file << "\n";
}

void after_exe(){
    ofstream file("output.txt", ios::app);
    if (!file.is_open()) {
        cerr << "Error opening file!" << endl;
        return;
    }

    file << curr_running_pid << " ";  // appends instead of overwriting
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

    if (PCB.empty()){
        debug("PCB is empty cannot get any process");
        return nullptr;
    }

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
    int highest_pl = RL.size() - 1;
    if (priority > highest_pl){
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

void delete_children(int parent_pid) {
    Process* parent = get_process(parent_pid);
    if (!parent) {
        debug("Trying to delete children of nullptr");
        return;
    }

    // Detach the children list to avoid iterator invalidation
    std::vector<int> children;
    children.swap(parent->children);   // parent->children becomes empty

    // Now safe: delete calls can mutate parent->children without affecting this loop
    for (int child_pid : children) {
        delete_process(child_pid);
    }
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


    if (is_proccess_0(target_pid)) {
        debug("Attempting to remove Process 0 from PCB");
        return;
    }
    erase_if(PCB, [target_pid](const Process& p){ return p.pid == target_pid; });
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

    auto& res = target->resources;
    for (auto it = res.begin(); it != res.end(); ) {
        int rid   = it->first;
        int units = it->second;
        ++it;                        // move iterator forward first
        release(rid, target_pid, units); // may erase rid safely now
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

    if (is_proccess_0(target_pid)){
        debug("Process 0 cannot be destroyed during runtime");
        return;
    }
    if (running_p -> has_child(target_pid) || target_pid == running_p -> pid){
        delete_process(target_pid);
        if (target_pid == curr_running_pid){
            scheduler();
        }
    }else{
        string msg = format("Running process {} can not destroy {} because it is not owner.", curr_running_pid, target_pid);
        debug(msg);
        return;
    }


}

Resource* get_resource(int target_rid){
    int highest_rid = static_cast<int>(RCB.size())-1;
    if (target_rid > highest_rid){
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

void rl_free_pids(int rid) {
    Resource* rr = get_resource(rid);
    if (!rr) {
        debug("RL_FREE_PIDS of nullptr impossible");
        return;
    }

    for (auto it = rr->WL.begin(); it != rr->WL.end(); ++it) {
        int pid   = it->first;
        int units = it->second;

        if (!rr->is_request_legal(units)) {
            continue;
        }

        Process* p = get_process(pid);

        if (!p) { 
            return;
        }

        int priority = p->priority;

        rr->alloc(units);            
        p->add_resource(rid, units);
        p->ready();

        // move PID from waiting list to ready queue
        // (assuming WL is your global vector<int> and RL[priority] is vector<int>)
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
    Arguments:
        given_pid: The PID of the process that will be releasing resource w/release_rid
        release_rid: The RID of the resource process w/given_pid will be releasing
        units: amount of Resource the process is trying to release

    Process w/given_pid is trying to release {units} amount of Resource w/release_rid
    */
    
    if (units < 0){
        debug("Attempting to release less than 0 resources.");
        return;
    }

    Process* given_p = get_process(given_pid);

    if(!given_p){ // Checks existance of PID
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

    given_p->remove_resource(release_rid, units);
    rr -> release(units);
    if (! rr -> WL.empty()){ // If WL has at least 1 process
        rl_free_pids(release_rid); // frees pids of free_pids
    }
    scheduler();
    string msg = format("Resource RID: {}, units: {}, has been released", release_rid, units);
    print(msg);
    
}

int highest_occupied_priority(){
    /*
    Retuerns the highest RL priority that actually has processes in it
    */

    for (int i = RL.size() - 1; i >= 0; --i){
        if (RL[i].empty()){
            continue;
        }else{
            return i;
        }
    }

    debug("Highest Occupied Priority returning -1");
    return -1;
}

void timeout(){
    /*
    Mimcks time sharing. Timesout moves the head of highest priority RL to the tail
    */
    
    Process* running_p = get_running_process();
    int run_priority = running_p -> priority;


    if (RL[run_priority].size() == 1){
        string msg = format("Only 1 process in RL[{}]. Just skip rotate.", run_priority);
        print(msg);
    }else{
        rotate(RL[run_priority].begin(), RL[run_priority].begin() + 1, RL[run_priority].end());
    }
    scheduler();

}

void scheduler(){
    /*
    Function in charge of context switching. Grab the highest priority process.
    */
    
    curr_running_pid = RL[highest_occupied_priority()][0];
    cout << "Process PID: " << curr_running_pid << " running." << endl;
    

}

void pcb_clear_but_0(){
    /*
    Clears the entire PCB except for process0
    */
    if (PCB.size() > 1) {
        PCB.erase(PCB.begin() + 1, PCB.end());
    }
}

void rl_clear_but_0(){
    /*
    Clears the ready list of all prioity levels but priority of 0
    */
    if (RL.size() > 1) {
        RL.erase(RL.begin() + 1, RL.end());
    }
}

void make_resources(int num_resources){
    /*
    Helper function for init. Used to create number of resources with default inventory
    First resource 0 takes special case 
        RID 0: inventory 1
    After all resources take properties of:
        RID r: inventory r
    */
    Resource r0 = Resource(0, 1);
    RCB.push_back(r0);
    for (int i = 1; i < num_resources; ++i){
        Resource r = Resource(i, i); // RID starts at 0, Size starts a 1
        RCB.push_back(r);
    }
}

void init(int levels, int num_resources){
    /*
    Returns the state of the program to the given levels and number of resources
    3 levels 4 resources 
    0 1 2
    0 1 2 3 4
    */
    detected_bug = false;
    pcb_clear_but_0();
    RCB.clear();

    rl_clear_but_0();
    WL.clear();

    RL.resize(levels);
    PCB[0].running();
    next_pid = 1;
    curr_running_pid = 0;
    
    make_resources(num_resources);
    

}

vector<string> get_tokens(const std::string& input){
    /*
    Gets all tokens for the command
    */
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
    Visual Check of what is truly happening in my program.
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

    for(size_t priority_level = 0; priority_level < RL.size(); ++priority_level){
        cout << "\tPriority: " << priority_level << endl << "\t\t";
        for (int pid: RL[priority_level]){
            cout << pid << " ";
        }
        cout << endl;
    }
    cout << endl;

    // Wait List
    cout << GREEN << "WL: " << NORMAL;
    for (int p : WL){
        cout << p << " ";
    }
    cout << endl;

}

void ru(vector<string> tokens){
    /*
    Each token represents the inventory of the selected resource.
    I.e

    1 1 2 3
    RID: 0 has inventory 1
    RID: 1 has inventory 1
    RID: 2 has inventory 2
    RID: 3 has inventory 3

    */

    RCB.clear();
    int curr_rid = 0;
    for (auto token : tokens){
        int units = stoi(token);
        Resource r = Resource(curr_rid++, units); // RID starts at 0, Size starts a 1
        RCB.push_back(r);

    }
}

void id(){
    /*
    Return the state of the program to the default state

    */
    init(3,4);
    vector<string> tokens = {"1", "1", "2", "3"};
    ru(tokens);
}

bool take_action(vector<string>& tokens){
    /*
    Returns: Wether or not the command was executed
    */
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
        release(rid, curr_running_pid, units);
        return true;
    }else if(command == "to"){
        timeout();
        return true;
    }else if(command == "in"){
        before_restart();
        int levels = stoi(tokens[1]);
        int num_r = stoi(tokens[2]);
        init(levels, num_r);
        return true;
    }else if(command == "see"){
        see_values();
        return false;
    }else if(command == "id"){
        before_restart();
        id();
        return true;
    }else if(command == "ru"){
        tokens.erase(tokens.begin());
        ru(tokens);
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

int line_skips = 1;

void print_line_skip(){
    cout << YELLOW << "LINE SKIP: " << line_skips++ << NORMAL << endl; 
}

void do_main() {
    clear_file("output.txt");
    vector<string> tokens;

    // --- First loop: wait for "id"
    for (;;) {
        string first_input = prompt();
        if (first_input == "q") return;
        if (first_input.empty()) {
            print_line_skip();
            continue;
        }
        tokens = get_tokens(first_input);
        if (tokens.empty()) continue;
        if (tokens[0] == "id") break;
    }
    print("Out of first loop");

    // --- Bootstrap
    Process p0 = Process(0, 0, -1, 0);
    PCB.push_back(p0);
    RL.resize(1);
    RL[0].push_back(0);
    id();
    after_exe();

    // --- Main REPL
    for (;;) {
        string input = prompt();
        if (input == "q") break;

        if (input.empty()) {
            print_line_skip();
            tokens.clear();
            continue;
        }

        tokens = get_tokens(input);
        if (tokens.empty()) {
            tokens.clear();
            continue;
        }

        if (take_action(tokens) && !detected_bug) {
            after_exe();
            tokens.clear();
            continue;
        }
        tokens.clear();

        if (detected_bug) {
            after_bug();

            // Wait until an "in" or "id" command
            for (;;) {
                input = prompt();
                if (input == "q") return;          // allow exit here too
                if (input.empty()) { print_line_skip(); continue; }

                tokens = get_tokens(input);
                if (!tokens.empty() && (tokens[0] == "in" || tokens[0] == "id")) {
                    break;
                }
            }

            if (take_action(tokens)) {
                tokens.clear();
                detected_bug = false;
                after_exe();
            }
        }
    }
}


static std::istream* g_in = &std::cin;

void set_input_stream(std::istream& is) {
    g_in = &is;
}

static inline void trim_inplace(std::string& s) {
    size_t i = 0, j = s.size();
    while (i < j && std::isspace(static_cast<unsigned char>(s[i]))) ++i;
    while (j > i && std::isspace(static_cast<unsigned char>(s[j - 1]))) --j;
    s.assign(s.begin() + i, s.begin() + j);
}

static int g_line_number = 0;
string prompt() {
    std::string line;

    if (g_in == &std::cin) std::cout << ">> " << std::flush;

    if (!std::getline(*g_in, line)) return "q";

    // Remove a trailing '\r' from Windows CRLF files (getline only strips '\n')
    if (!line.empty() && line.back() == '\r') line.pop_back();

    // Also trim outer whitespace so "id  " or "to " still match
    trim_inplace(line);
    ++g_line_number;
    cout << BLUE <<"[LINE " << g_line_number << "] " << line  << NORMAL << endl;
    return line;
}

int main() {
    std::ifstream file("input.txt");
    if (file) {
        set_input_stream(file);  // feed lines from file first
    } else {
        std::cerr << "Warning: input.txt not found; using interactive input.\n";
    }
    print("Doing main");
    do_main();
    return 0;
}
