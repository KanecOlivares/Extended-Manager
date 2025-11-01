#ifndef PROCESS_H
#define PROCESS_H

#include <vector>
#include <algorithm>
#include <iostream>
#include <string.h>
#include <map>
using namespace std;

string NORMAL = "\033[0m";
string RED = "\033[31m";

void warning(string msg){
    cout << RED << msg << NORMAL  << " FROM PROCESS.H" << endl;
}

class Resource{
    public:
        int rid; // Resource ID
        int state; // Num of available resources
        int inventory; // inital number of resources
        map<int, int> WL; // PID, units requested

        Resource(int r, int inital_units) : rid(r), state(inital_units), inventory(inital_units){}

        bool is_alloc(){
            /*
            Checks if it is fully allocated
            */
            
            if (state < 0){
                warning("State got to less than zero.");
            }
            return state == 0;
        }

        bool is_free(){
            /*
            Check if there are any resouces free.
            */
            return state > 0;
        }

        bool is_request_legal(int units){
            /*
            Logical Check to see if amount request is feasiable 
            */
            return state - units >= 0;
        }

        void alloc(int units){
            /*
            Positive units means they are able to get the resource and negative means they are waiting for resource
            When equalling to zero I have to be able to make them free
            */

            if (is_request_legal(units)){ 
                state -= units;
            }else{ // if units > available units
                warning("Trying to alloc more than avaialble.");
                return;
            }
        }

        bool is_release_legal(int units){
            /*
            Checks if it is legal to release unit amount of resources. 
            */
            return state + units <= inventory;
        }

        void release(int units){
            
            if (is_release_legal(units)){
                state += units;
            }else{
                warning("Trying to release too many units. Excceds inventory");
                return;
            }
            
        }

        vector<int> now_free(){
            /*
            Returns a vector of PIDS that can now be free in current state of Resource
            */
            vector<int> del_wl_pid;
            for (auto& [pid, units_requested] : WL){
                if (units_requested <= state){
                    del_wl_pid.push_back(pid);
                }
            }
            return del_wl_pid;
        }

        void add_wl_process(int pid, int units){
            /*
            PID is now a blocked process which is requesting {units} amount of resource
            */
            if (WL.contains(pid)){
                string msg = format("Process {} is already in WL. Cannot be requesting resources if it is a blocked proccess. Requesting: {} units", pid, units);
                warning(msg);
            }else{
                WL[pid] = units;
            }
        }

        void force_free(){
            /*
            Forces the resouce into a free state.
            */
            state = inventory;
            WL.clear();
        }

        void print(){
            string msg = format("RID: {}, state: {}, inventory: {}", rid, state, inventory);
            cout << msg << endl << "WL: " << endl;
            for (const auto &[key, value] : WL){
                cout << "\t" << "PID: " << key << " Units: " << value << endl;
            }
            cout << endl;
        }

    };

class Process{
    public:

    int pid;
    int state; // 0: ready, 1: running, 2: blocked
    int parent;
    int priority;

    vector<int> children; // Vector of PIDs of children
    map<int, int> resources; // RID, amount units held by Process

    // Destructor
    ~Process(){
        state = -1;
        parent = -1;
        // Children will be deleted automaticallt using vector
        // resources will be deleted automatically using vector
    }

    // Default Constructor
    Process() : state(-1), parent(-1){ }

    // Constructor params
    Process(int id, int s, int p, int pr): pid(id), state(s), parent(p), priority(pr){ }   

    void add_child(int child_pid){
        children.push_back(child_pid);
    }

    bool has_child(int pid){
        /*
        Check if the process had child with given pid
        */
        auto it = find(children.begin(), children.end(), pid);
        return it != children.end();
    }

    void remove_child(int child_pid){
        /*
        Removes the child from the childlist
        Key notes: remove does the shifting. All elements to the right of the
        target are shifted towards the begining(left).
        Erase: Takes care of the garbage tail. And decrements size.
        */

        children.erase(remove(children.begin(), children.end(), child_pid), children.end());
    }

    void add_resource(int rid, int units){
        /*
        Adds unit amount of RID resource.
        */
        if (resources.contains(rid)){
            resources[rid] += units;
        }else{
            resources[rid] = units;
        }
    }

    void ready(){
        state = 0;
    }

    void running(){
        state = 1;
    }

    void block(){
        state = 2;
    }

    bool has_resource(int rid, int units){
        /*
        Checks if Process even has resource. Then checks if process has units 
        amount of resource.

        IMPORTANT: Used to check in cases like releasing a resource it has but 
        more than it actuall has of that respurce
        */
        return resources.contains(rid) && units <= resources[rid];
    }

    void remove_resource(int rid, int units){
        /*
        Removes units amount of RID resource. In the case that it removes 
        all units of that resource it will then take it off it its resource
        list.
        */
        if (has_resource(rid, units)){
            int new_units = resources[rid] - units;
            if (new_units == 0){
                resources.erase(rid);
            }else{
                resources[rid] = new_units;
            }
        }else{
            warning("Trying to release too many units.");
        }
    }

    void print(){
        string msg = format("PID: {}, state: {}, parent: {} ", pid, state, parent);
        cout << msg << endl << "Resources: " << endl;
        for (const auto &[key, value] : resources){
            cout << "\t" << "RID: " << key << " Units: " << value << endl;
        }
        cout << endl;
    }
};

#endif
