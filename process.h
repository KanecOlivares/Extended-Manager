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
            if (state < 0){
                warning("State got to less than zero.");
            }
            return state == 0;
        }

        bool is_free(){
            return state > 0;
        }

        bool is_request_legal(int units){
            return state - units >= 0;
        }

        void alloc(int units){
            /*
            Positive units means they are able to get the resource and negative means they are waiting for resource
            When equalling to zero I have to be able to make them free
            */

            if (is_request_legal(units)){ // if units > available units
                state -= units;
            }else{
                warning("Trying to alloc more than avaialble.");
                return;
            }

            
        }



        void release(int units){
            int new_avail_units = state + units;
            if (new_avail_units > inventory){
                warning("Trying to release too many units. Excceds inventory");
                return;
            }
            state = new_avail_units;
        }

        vector<int> now_free(){
            /*
            Returns vector of PIDS that can now be free
            */
            vector<int> del_wl_pid;

            for (auto& [key, value] : WL){
                if (value <= state){
                    del_wl_pid.push_back(key);
                }
            }

            // for (int pid : del_wl_pid){
            //     WL.erase(pid);
            // }

            return del_wl_pid;
        }

        void add_wl_process(int pid, int units){
            /*
            PID is now a blocked process which is requesting {units} amount of resource
            */
            if (WL.contains(pid)){
                string msg = format("{} is already in WL. Cannot be requesting resources if it is blocked. Requesting: {} units", pid, units);
                warning("Already in WL. Impossible due to blocked processes not being able to request resources");
            }else{
                WL[pid] = units;
            }
        }

        void force_free(){
            state = inventory;
            WL.clear();
        }

        void print(){
            string msg = format("RID: {}, state: {}, inventory: {}", rid, state, inventory);
            cout << msg << endl << "WL: ";
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

    
    // ChildList children; // LL of process it created (children processes)
    vector<int> children; // Vector of PIDs of children

    map<int, int> resources; // map RID, units

    // Destructor
    ~Process(){
        state = -1;
        parent = -1;
        // Children will be deleted using ~ChildList()
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
        auto it = find(children.begin(), children.end(), pid);
        return it != children.end();
    }

    int num_children(){
        return children.size();
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
        return resources.contains(rid) && units <= resources[rid];
    }

    void remove_resource(int rid, int units){
        /*
        Removes resouce, units pair
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
        cout << msg << endl << "Resources: ";
        for (const auto &[key, value] : resources){
            cout << "\t" << "RID: " << key << " Units: " << value << endl;
        }
        cout << endl;
    }
};

#endif



// Resource functions

        // void free(){
        //     if (state == 1){
        //         warning("It is already free");
        //     }
        //     if (!WL.empty()){
        //         warning("Waitlist is not empty. Impossible to be free!");
        //         return;
        //     }if (state <= 0){
        //         warning("Nothing in inventory. Impossible to be free!");
        //         return;
        //     }
        //     state = inventory;
        // }

                // bool release_units(int units){
        //     int new_count = inventory - units;
        //     if (new_count >= 0){
        //         inventory = new_count;
        //         return true;
        //     }
        //     return false; 
        // }
