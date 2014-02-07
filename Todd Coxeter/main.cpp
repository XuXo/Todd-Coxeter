// toddCoxeter.cpp : Defines the entry point for the console application.
// This parses a presentation (set of generators and relators to find all elements of a group, todd coxeter is a nascent approach to tackling the word problem,
// see references listed below right now doesn't allow input requires user to specify number of generators since vector<array> was the easiest way to do this logically, need to change this later.  Heavily commented for debugging purpose.
// http://en.wikipedia.org/wiki/Todd–Coxeter_algorithm
// http://www.math.cornell.edu/~kbrown/7350/toddcox.pdf
// http://web.science.mq.edu.au/~chris/groups/CHAP05%20The%20Todd-Coxeter%20Algorithm.pdf


//#include "stdafx.h"		//vs2010 dependent
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stdexcept>

#include <iostream>
#include <fstream>

#include <array>
#include <vector>
#include <string>
#include <math.h>
#include <map>
#include <sstream>

#define NUMGEN 2
#define VERBOSE
#define DEBUG2

using namespace std;

template <class T>
inline std::string to_string (const T& t)
{
    std::stringstream ss;
    ss << t;
    return ss.str();
}

class todd_coxeter
{
	typedef vector<string> vs;
	typedef vector<vector<string>> vss;
	vs generators;
	vs relators;
	vs chains;
	std::vector<std::array <int, 2> > links;
	map<string, int> suffix_chains;
    ofstream myfile;
    
public:
	todd_coxeter ();
	void init ();
	void start ();
	string indexToGenerator (int i);
	int generatorToIndex (string gen);
	void reduceChains ();
    void expandLinks ();
	bool linksFull ();
	void updateNextPosition (int& starting_index, int code);
	void insertLink (string link);
    bool cleanChains();
	int cur_largest_index ();
	void printLinks ();
	void printChains ();
};

todd_coxeter::todd_coxeter() {
	myfile.open("testing.txt");
}



//will take generators and relations and construct the basic object
void todd_coxeter::init() {
    
	generators.push_back("a");
	generators.push_back("b");
    
	relators.push_back("aaa");							//all relators are implicitly equal to the identity element
	relators.push_back("bb");
	relators.push_back("baba");
    
}

void todd_coxeter::start() {
    
	int num_generators = generators.size();
    
	int num_gen = generators.size();
    
    
	std::array<int, 2> dummy;                           //this is so we can refer to 1B2 to really mean 1B2 since index starts at 0
	dummy.fill(-1);
	links.push_back(dummy);
	
	std::array<int, 2> one;                             //initially start with just one row or index '1'
	one.fill(-1);
	links.push_back(one);
    
	//std::array<int, 2> two;
	//two.fill(-1);
	//links.push_back(two);
    
	printLinks();
    
    
	int kth_unknown = 0;
	int starting_index = 1;
    
	//creating the first set of chains with identity element '1'
	for(int i = 0; i<relators.size();i++)
        chains.push_back(std::to_string(starting_index) + relators[i] + std::to_string(starting_index));
    
    
	while(!linksFull())
    {
		int cur_largest = cur_largest_index();								//on first iteration this will return 1 so we insert 2
		int new_code = cur_largest + 1;
        
#ifdef DEBUG2
		cout<<"new code to be inserted is"<<new_code<<endl;
#endif
        
        //insert into next available position in links
		updateNextPosition(starting_index, new_code);
        
        //so starting_index is where we are now, new code is not always just plus one
        //ie consider the following
        //				 A	  B
        //			1	(2)	 (3)
        //here 1B3 so we are going from 1 to 3
        
		printLinks();
        
		for(int i =0; i<relators.size();i++)
			chains.push_back(std::to_string(new_code) + relators[i] + std::to_string(new_code));
        
		printChains();
        
		//now concactenate what we can and reduce the set of chains
		reduceChains();
        
	}
    cout<<"########################################################################################################"<<endl;
	cout<<"                       links table completed, now can construct the group table                         "<<endl;
    cout<<"########################################################################################################"<<endl;
    
	printLinks();
}


//updates the links table based on next available position
void todd_coxeter::updateNextPosition (int& starting_index, int code) {
    
#ifdef VERBOSE
	cout<<"starting index is "<<starting_index<<endl;
#endif
	bool brake = false;;
	for(int i = starting_index; i<links.size(); i++) {
		for(int j = 0; j<links[i].size(); j++) {
			if(links[i][j] == -1) {
				//if we are at the last element of the row, we need to update starting_index to go downward on next iteration, update is left to right top to bottom
				if (j == links[i].size() - 1) {
					starting_index++;
#ifdef VERBOSE
					cout<<"incremented startingindex"<<endl;
#endif
				}
				links[i][j] = code;		//ie at very first iteration where starting_index = 1, first opening gives 1A2 so we put 2 in links[0][0];
                expandLinks();
				string suffix = to_string(indexToGenerator(j)) + to_string(code);		//if 1B2, insert as ("B2", 1)
				
#ifdef VERBOSE
				cout<<"link created is "<<i<<suffix<<endl;
#endif
                
				suffix_chains.insert(pair<string, int>(suffix, i));
				brake = true;
				break;
			}
		}
		if(brake)
			break;
	}
}


//new links constructed as a result of reduction as opposed to updateNextPosition() which is user defined
void todd_coxeter::insertLink (string link) {
    
#ifdef VERBOSE
    cout<<"Inserting the new link : "<<link<<endl;
#endif
    
	int prefix_index = stoi( link.substr(0,1));
	int prefix_gen = ( generatorToIndex(link.substr(1,1)));
	int code = stoi( link.substr(2,1));
	if(links[prefix_index][prefix_gen] == -1) {
		links[prefix_index][prefix_gen] = code;
        string suffix = to_string(indexToGenerator(prefix_gen)) + to_string(code);		//if 1B2, inserted as ("B2", 1)
        suffix_chains.insert(pair<string, int>(suffix, prefix_index));
        printLinks();
    }
	else if( links[prefix_index][prefix_gen] == code) {
#ifdef VERBOSE
		cout<<"Link already exists"<<endl;
#endif
	}
	else{
#ifdef VERBOSE
		cout<<"we have a problem needs to backup"<<endl;
#endif
	}
	//need some kind of conflict resolution
    
}


//remove links(length 3) from chains
bool todd_coxeter::cleanChains() {
	bool cleaned = false;
    
#ifdef VERBOSE
    cout<<"removing chains of length 3 from the list "<<endl;
#endif
    for(int i = 0; i<chains.size(); i++)
        if(chains[i].length() == 3) {
            insertLink(chains[i]);
			chains.erase(chains.begin() + i);
			cleaned = true;
			i--;
			
		}
    printChains();
    return cleaned;
}


string todd_coxeter::indexToGenerator (int i) {
    
	return generators[i];
}


int todd_coxeter::generatorToIndex (string gen) {
    
	for (int i = 0; i<generators.size(); i++){
		if(generators[i] == gen)
			return i;
	}
	return -1;
}


//chain reductio on prefix and suffix, prefix 1A is to be replaced by 2 if and only if 1A2 is a defined link, likewise suffix A2 is to be replaced by 1.
void todd_coxeter::reduceChains() {
    
    //if reduced during current iteration we will need to rescan
	bool reduced = false;
    
#ifdef VERBOSE
	cout<<"-------------reducing the prefix first -------------"<<endl;
#endif
	//reduce the prefix
	for (int i = 0; i < chains.size(); i++) {
        
#ifdef VERBOSE
		cout<<"\nlooking at chain : "<<chains[i]<<endl;
#endif
		int prefix_index = stoi( chains[i].substr(0,1));
		
		string gen = chains[i].substr(1,1);
		int prefix_gen = generatorToIndex(gen);
		//cout<<"corresponding index of generator is "<<prefix_gen<<endl;
		//cout<<"prefix to be reduced is "<<prefix_index<<gen<<endl;
        
		if (links[prefix_index][prefix_gen] != -1) {
			int result = links[prefix_index][prefix_gen];
			chains[i].replace(0,2,to_string(result));
            
#ifdef VERBOSE
			cout<<"the link is defined so we will be replacing prefix with "<<result<<endl;
			cout<<"reduced chain is "<<chains[i]<<endl<<endl;
#endif
            reduced = true;
		}
	}
    
	printChains();
    
    
	//we might not need this here since all links should be captured towards the end which will kick off a rescan
	//actually we do, a link by def should not be reduced ie 1a2 will become 22 trivially....
	
	//need to update links if we created any after prefix reduction
	/*int inc = 1;
     for (int i = 0; i < chains.size(); i++)
     if(chains[i].length() == 3){
     //if we got a new link, we insert it in the links table and reduce once again
     insertLink(chains[i]);                  //insert the link
     cleanChains();
     reduced = true;
     //when we remove a link from chains, everything after this ekement gets reindexed to i-1 so we could potentially another link if they are back to back
     i--;
     }*/
	
    
	//removing links from chains, otherwise suffix might turn links into 2 digit nonsense
	if(cleanChains())
		reduced = true;
    
	//reduce the suffix
#ifdef VERBOSE
	cout<<"-------------now reducing the suffix -------------"<<endl;
#endif
	for (int i = 0; i < chains.size(); i++) {
        
#ifdef VERBOSE
		cout<<"\nlooking at chain : "<<chains[i]<<endl;
#endif
		int len = chains[i].size();
		//string suffix_index = chains[i].substr(len-1, 1);
		//string gen = chains[i].substr(len-2, 1);
		string suffix = chains[i].substr(len-2, 2);
		//cout<<"consider the suffix "<<suffix<<endl;
		if(suffix_chains.find(suffix) == suffix_chains.end()){
			//cout<<"The suffix "<<suffix<<" is not in the map"<<endl;
		}
		else{
            
			chains[i].replace(len-2,2, to_string((suffix_chains.find(suffix))->second));
			string code = to_string((suffix_chains.find(suffix))->second);
            
#ifdef VERBOSE
			cout<<"the suffix is defined and we will be replacing suffix with "<<code<<endl;
            cout<<"reduced chain is "<<chains[i]<<endl<<endl;
#endif
			reduced = true;
		}
	}
	printChains();
#ifdef VERBOSE
	cout<<"-------------------\none iteration of reduce completely done\n--------------------"<<endl;
#endif
    
	/*for (int i = 0; i < chains.size(); i++)
     if(chains[i].length() == 3){
     //if we got a new link, we inser it in the links table and reduce once again
     insertLink(chains[i]);                  //insert the link
     cleanChains();                          //remove from chains which are only chains of length >= 4
     cout<<"chains of length 3 have been removed\n";
     reduced = true;
     //reduceChains();
     }*/
    
	if(cleanChains())
		reduced = true;
    
	if(reduced){
#ifdef VERBOSE
		cout<<"rescanning after links updated\n\n";
#endif
		reduceChains();								//redo
		
	}
	else{
#ifdef VERBOSE
		cout<<"----------------------------------------------------------------------------------"<<endl;
		cout<<" ------------------we're done for now, let's add a new link!----------------------"<<endl;
#endif
	}
}

//add new row to links
void todd_coxeter::expandLinks () {
    
    int cur_largest = cur_largest_index();								//on first iteration this will return 1 so we insert 2
    int numrows = links.size() - 1;
    
    if(cur_largest > numrows) {
        std::array<int, 2> newrow;				//this is so we can refer to 1B2 to really mean 1B2 since index starts at 0
        newrow.fill(-1);
        links.push_back(newrow);
#ifdef VERBOSE
        cout<<"we just appended a new row don't worry!";
#endif
    }
}


//check whether links is complete
bool todd_coxeter::linksFull () {
	for(int i = 1; i<links.size(); i++) {
		for(int j = 0; j<links[i].size(); j++)
			if(links[i][j] == -1){
                cout<<"it's not full at"<<i<<","<<j<<endl;
				return false;
            }
        cout<<endl;
	}
	return true;
}


//return current largest defined link
int todd_coxeter::cur_largest_index() {
	int max = 1;
	for(int i = 0; i<links.size(); i++) {
		for(int j = 0; j<links[i].size(); j++)
			if(links[i][j] > max)
				max = links[i][j];
	}
	return max;
}


void todd_coxeter::printLinks() {
    
#ifdef VERBOSE
	cout<<"\nCurrent links table:"<<endl;
#endif
	for (int i = 0; i<links.size(); i++) {
		for (int j = 0; j < links[i].size(); j++)
			cout<<links[i][j]<<" ";
		cout<<endl;
	}
	cout<<endl;
}


void todd_coxeter::printChains() {
    
#ifdef VERBOSE
	cout<<"\nCurrent set of chains: "<<endl;
#endif
	for(int i = 0; i<chains.size(); i++)
		cout<<chains[i]<<endl;
	cout<<endl;
}


int main(int argc, const char *argv[])
{
	todd_coxeter presentation;
	presentation.init();
	presentation.start();
    
	cin.get();
}



