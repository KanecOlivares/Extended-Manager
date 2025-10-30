#include <vector>
#include <array>
#include <sstream>
#include <algorithm>
#include <format> // Require CPP +20
#include <string>
#include <fstream>
#include <iostream>
#include "process.h"
using namespace std;

string GREEN = "\033[32m";
string NORMAL = "\033[0m";

vector<Process> PCB;
int max_pcb = 16;
vector<Resource> RCB;

// Ready and Waitl ist
// They only store the pid of their respective processes
vector<int> RL; 
vector<int> WL;

// Declarations for compiler
Process* get_process(int pid);
void delete_process(Process* target);
void release(int release_rid, Process* given_p);
void scheduler();

int next_pid = 1;

void debug(string msg){
    cout << "ERROR: ";
    cout << msg << endl;
}

Process* get_running_process(){
    /*
    Return the Pointer to running process
    */

    if (RL.empty()){
        debug("Ready list is empty.");
        return nullptr;
    }
    int run_pid = RL[0];
    return get_process(run_pid);
}

void after_exe(){
    ofstream file("output.txt");
    if (!file.is_open()) {
        cerr << "Error opening file!" << endl;
        return;
    }
    Process* running_p = get_running_process();
    int run_pid = running_p -> pid;
    file << run_pid << " "; 
    file.close();
}

void print(string msg){
    cout << msg << endl;
}

bool is_proccess_0(Process* p){
    if (!p){
        debug("nullptr cannot be process 0");
        return false;
    }
    return p -> pid == 0;
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



void add_child_to(Process* parent, Process* child){
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

void create(){
    /*
    Running PID creates the child. Use next_pid and post increment for the next create()
    */

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
    Process child = Process(next_pid++, 0, run_pid); // pid, state (0 = ready), parent_pid
    PCB.push_back(child);

    Process* parent = get_running_process();
    Process* child_ptr = get_process(child.pid);

    add_child_to(parent, child_ptr);
    RL.push_back(child_ptr -> pid);

    string msg = format("Process {} created", child.pid);
    print(msg);

}

void delete_children(Process* parent){
    /*
    For all children in the parent's child vector delete those aswell
    */

    if (!parent){
        debug("Trying to delete children of nullptr");
        return;
    }

    for (int child_pid : parent -> children){
        Process* child = get_process(child_pid);
        if (!child){
            debug("Child is null ptr");
            return;
        }
        delete_process(child);
    }
    parent -> children.clear();

    
}

void remove_from_PCB(Process* target){
    /*
    Removes the target process* in PCB
    Had to do remove_if b/c there was a mismatch.

    Compile error:
    std::remove<std::__wrap_iter<Process *>, Process>' requested here 155 | 
        PCB.erase(remove(PCB.begin(), PCB.end(), *target),

    Solution: Use remove_if and use the pids.
    */

    if (!target){
        debug("Attempting to remove nullptr from PCB");
        return;
    }
    if (is_proccess_0(target)){
        debug("Attempting to remove Process 0 from PCB");
        return;
    }
    int pid = target->pid;
    PCB.erase(remove_if(PCB.begin(), PCB.end(),
                         [pid](const Process& p){ return p.pid == pid; }),
          PCB.end());

}

void remove_from_parent_list(Process* child){
    /*
    Given a child Process* it will obtain the parent process* and
    remove the child from parent.childlist
    */

    if (!child){
        debug("Attempting to remove from parent list. Child is nullptr ");
        return;
    }
    Process* parent = get_process(child->parent);
    if (!parent){
        debug("Parent is nullptr. Cannot remove child from nullptr");
        return;
    }

    parent->remove_child(child);
}

void remove_from_lists(Process* target){
    /*
    Removes the target from RL or WL. It is safe due to erase(remove()) does
    nothing if it does't exist in the waitlist
    */
    
    if (!target){
        debug("Cannot remove nullptr from any list (WL or RL)");
        return;
    }
    int target_pid = target -> pid;

    RL.erase(remove(RL.begin(), RL.end(), target_pid), RL.end());
    WL.erase(remove(WL.begin(), WL.end(), target_pid), WL.end());
}

void release_resources(Process* target){
    /*
    Release all the resources of target
    */
    if (!target){
        debug("Cannot release resources of nullptr");
        return;
    }
    for (int rid : target -> resources){
        release(rid, target);
    }
}

void delete_process(Process* target){
    /*
    Delete Children first then delete target
    */

    if (is_proccess_0(target)){
        print("Skipping Deletion of Process 0");
        return;
    }

    if (!target){
        debug("Attempted to delete nullptr");
        return;
    }

    if (! target -> children.empty()){
        delete_children(target);
    }

    if(target -> parent > 0){
        remove_from_parent_list(target); // removes pid from children vector
    }

    remove_from_lists(target); // removes from RL or WL which ever they are in
    release_resources(target);
    remove_from_PCB(target);
    int target_pid = target -> pid;
    delete target;
    string msg = format("Destroyed process with pid {}", target_pid);
    print(msg);
}

void destroy(int pid){
    Process* running_p = get_running_process();
    if (!running_p){
        debug("Nullptr cannot destroy any processes");
        return;
    }
    if (pid == 0){
        debug("Process 0 cannot be destroyed during runtime");
        return;
    }
    if (running_p -> has_child(pid) || pid == running_p -> pid){
        Process* p = get_process(pid);
        delete_process(p);
    }else{
        string msg = format("Running process {} can not destroy {} because it is not owner.", running_p -> pid, pid);
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

void move_process_to_other(vector<int>& from, vector<int>& to, Process* target){
    /*
    Erase the PID from the from list and move to the to list. 
    */
    int target_pid = target->pid;
    from.erase(remove(from.begin(), from.end(), target_pid), from.end());
    to.push_back(target_pid);
}

void request(int request_rid){
    /*
    Running process requesting resource with rid = request_rid
    */
    Process* runnning_p = get_running_process();
    if (!runnning_p){
        debug("Nullptr can not request a resource.");
        return;
    }

    Resource* rr = get_resource(request_rid);  // Requested Resource ptr
    if (!rr){
        debug("Requested resource does not exist.");
        return;
    }

    if(is_proccess_0(runnning_p)){
        debug("Process 0 can not request any resources.");
        return;
    }

    if (runnning_p -> has_resource(request_rid)){
        string msg = format("Running process: {} already hold resource: {}.", runnning_p -> pid, request_rid);
        debug(msg);
    }

    if (rr->is_free()){
        rr->alloc();
        runnning_p->add_resource(request_rid);
        string msg = format("Resource with rid: {} has been allocated to process with pid {}", request_rid, runnning_p ->pid);
        print(msg);

    }else{ // Reousrce is not free
        runnning_p -> block();
        move_process_to_other(RL, WL, runnning_p); // Move from RL to WL the process being moved is running_p
        string msg = format("Process with PID: {} is now bloacked", runnning_p -> pid);
        print(msg);
        scheduler();
    }
}

void release(int release_rid, Process* given_p){
    /*
    Releases resource w/rid from given_p.
    */

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

    if (! given_p-> has_resource(release_rid)){ // given program is not owner of resource
        string msg = format("Given Process PID: {} does not own resouce {}", given_p -> pid, release_rid);
        debug(msg);
        return;
    }

    given_p->remove_resource(release_rid);

    if (rr -> WL.empty()){
        rr -> free();
        return;
    }else{

        int next_ready_pid = rr -> pop_WL_front(); // returns and erases head
        Process* next_ready_process = get_process(next_ready_pid);
        move_process_to_other(WL, RL, next_ready_process);
        next_ready_process -> ready();
        next_ready_process -> add_resource(release_rid);

    }
    string msg = format("Resource RID: {}, has been released", release_rid);
    print(msg);
    
}

void timeout(){
    /*
    Mimcks time sharing

    */
    if (RL.size() == 1){
        print("Only 1 process in ready list. Just skip");
        return;
    }

    Process* running_p = get_running_process();
    rotate(RL.begin(), RL.begin() + 1, RL.end());
    running_p -> ready(); // Now it is the old running process is ready()
    Process* new_running_p = get_running_process();
    new_running_p -> running(); // New running_p is now running()

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

void init(){
    pcb_clear_but_0();

    RL.clear();
    PCB[0].running();
    RL.push_back(PCB[0].pid);

    WL.clear();
    force_free_all(); // all resources

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

}

bool take_action(vector<string>& tokens){
    string command = tokens[0];
    if (command == "cr"){
        // print("Made it to create()");
        create();
        return true;
    }else if (command == "de"){
        int pid = stoi(tokens[1]);
        destroy(pid);
        return true;
    }else if(command == "rq"){
        int rid = stoi(tokens[1]);
        request(rid);
        return true;
    }else if(command == "rl"){
        int rid = stoi(tokens[1]);
        Process* p = get_running_process();
        release(rid, p);
        return true;
    }else if(command == "to"){
        timeout();
        return true;
    }else if(command == "in"){
        init();
        return true;
    }else if(command == "dbg"){
        see_values();
        return true;
    }
    print("Not a valid command");
    return false;

}

// int main(){
//     string input;
//     vector<string> tokens;
//     Process p0 = Process(0, 0, -1); // PID; 0, state = 0 (ready), no parent 
//     PCB.push_back(p0);
//     init();
//     while(true){
//         input = prompt();

//         if (input == "q"){
//             break;
//         }
        
//         tokens = get_tokens(input);
//         if (take_action(tokens)){
//             tokens.clear();
//             after_exe();
//         }else{
//             input = prompt();
//             tokens = get_tokens(input);
//             while (tokens[0] != "in" || tokens[0] != "id"){
//                 input = prompt();
//                 tokens = get_tokens(input);
//             }
//         }
//     }
// }

int main() {
    
    vector<std::string> tokens;

    Process p0 = Process(0, 0, -1); // PID 0, ready, no parent
    PCB.push_back(p0);
    init();

    for (;;) {
        string input = prompt();
        if (input == "q") break;

        tokens = get_tokens(input);
        if (tokens.empty()){
            debug("No tokens detected");
            continue;
        };

        if (take_action(tokens)) {
            tokens.clear();
            after_exe();
            continue;
        }

        // If action not taken, require a command starting with "in" or "id"
        for (;;) {
            input = prompt();
            tokens = get_tokens(input);
            if (!tokens.empty() && (tokens[0] == "in" || tokens[0] == "id")) {
                break; // got a valid starter token
            }
            // optionally print a hint:
            // std::cout << "Enter a command starting with 'in' or 'id'\n";
        }

        // Now handle that command:
        if (take_action(tokens)) {
            tokens.clear();
            after_exe();
        }
    }
}


