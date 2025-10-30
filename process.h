#ifndef PROCESS_H
#define PROCESS_H

#include <vector>
#include <algorithm>
#include <iostream>
#include <string.h>
using namespace std;

void warning(string msg){
    cout << msg << endl;
}

class Resource{
    public:
        int rid; // Resource ID
        int state; // 0: Allocated, 1: Free
        vector<int> WL;

        bool is_alloc(){
            return state == 0;
        }

        bool is_free(){
            return state == 1;
        }

        void alloc(){
            if (state == 0){
                warning("It is already allocated");
                return;
            }
            state = 0;
        }

        void free(){
            if (state == 1){
                warning("It is already free");
            }
            if (!WL.empty()){
                warning("Waitlist is not empty. Impossible to be free!");
                return;
            }
            state = 1;
        }

        int get_WL_head(){
            return WL[0];
        }

        void release_head(){

            if (WL.empty()){
                warning("Can not relase process from WL becasue it is empty");
                return;
            }

            WL.erase(WL.begin());

        }

        int pop_WL_front(){
            int head = get_WL_head();
            release_head();
            return head;
        }

        void force_free(){
            state = 1;
            WL.clear();
        }

        void print(){
            string msg = format("RID: {}, state: {}, inventory: {}", rid, state, -1);
            cout << msg << "WL: ";
            for (auto r : WL){
                cout << r << " ";
            }
            cout << endl;
        }

    };

class Process{
    public:
    int pid;
    int state; // 0: ready, 1: running, 2: blocked
    int parent;
    // ChildList children; // LL of process it created (children processes)
    vector<int> children; // Vector of PIDs of children

    vector<int> resources; // LL of resources it is holding

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
    Process(int id, int s, int p): pid(id), state(s), parent(p){ }   

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
    void remove_child(Process* child){
        /*
        Removes the child from the childlist
        Key notes: remove does the shifting. All elements to the right of the
        target are shifted towards the begining(left).
        Erase: Takes care of the garbage tail. And decrements size.
        */
        if (!child){
            warning("Trying to remove child from parent list when child is nullptr.");
            return;
        }

        children.erase(remove(children.begin(), children.end(), child->pid), children.end());
    }
    void add_resource(int rid){
        resources.push_back(rid);
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

    bool has_resource(int rid){
        auto it = find(resources.begin(), resources.end(), rid);
        return it != resources.end();
    }

    void remove_resource(int rid){
        /*
        Removes resouce from process resources
        */
        resources.erase(remove(resources.begin(), resources.end(), rid), resources.end());
    }

    void print(){
        string msg = format("PID: {}, state: {}, parent: {} ", pid, state, parent);
        cout << msg << endl << "Resources: ";
        for (auto r : resources){
            cout << r << " ";
        }
        cout << endl;
    }
};

#endif