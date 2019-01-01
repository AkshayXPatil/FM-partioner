/*******************************************
EEDG6375 Design Automation of VLSI systems
Programming assignment 1, FM partitioner
by,
		Team-9
Akash Anil tadmare		aat170130
Akshay Nandkumar Patil	anp170330
********************************************/

#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <map>
#include <algorithm>
#include <cstdlib>
using namespace std;


//global declarations

int sumA = 0;
int sumB = 0;
int sumA_mincut = 0;
int sumB_mincut = 0;

int cutset =0;
int new_cutset = 0;
int new_index = 0; //index for selecting next cell if balance_check not satisfied
int lock_indx=0;
int i =0;
int j =0;
int k =0;
int mincut = 0;
int startcut = 0;
int old_partition =0;

vector <int> locked_cells;
map <int, vector <int> > bucketA;
map<int, vector<int> >::iterator indexA = bucketA.end(); //index for gainA

//ofstream logfile ("FM.log");
ofstream normal ("normal.txt");
ofstream minfile ("minfile.txt");
vector<string> split(string strToSplit, char delimeter)
{
    stringstream ss(strToSplit);
    string item;
    vector<string> splittedStrings;
    while (getline(ss, item, delimeter))
    {
       splittedStrings.push_back(item);
    }
    return splittedStrings;
}

int toint(string strToint)
{
	stringstream ss(strToint);
	int item = 0;
	ss>>item;
	return item;
}

class cell
{
	public:
	int cell_size;
	int FS;
	int TE;
	int cell_gain;
	int lock_status;// 1=locked, 0=unlocked
	string cell_type;
	int cell_partition;  //A = 0,  B = -1
	//string cell_partition;
	vector <string> net_list;
	//vector <string> cell_neighbours;
};

class net
{
	public:
	vector <int> cell_list;	

	int cutstate; //0 = uncut, -1 = cut
	int Asize;
	int Bsize;
	int lock_count;
	bool critical;
};

//class type global declaration 
map <int, cell> cell_map;
map <string, net> net_map;

 
//balance check function
bool balance_check(int cell_to_move)
{
	int temp_sumA = sumA;
	int temp_sumB = sumB;
	if (cell_map[cell_to_move].cell_partition == 0)
	{
		temp_sumA = temp_sumA - cell_map[cell_to_move].cell_size;
		temp_sumB = temp_sumB + cell_map[cell_to_move].cell_size;
	}
	else if (cell_map[cell_to_move].cell_partition == -1)
	{
		temp_sumA = temp_sumA + cell_map[cell_to_move].cell_size;
		temp_sumB = temp_sumB - cell_map[cell_to_move].cell_size;
	}
	
	if(abs(temp_sumA - temp_sumB) <= 860 )
	{
		sumA = temp_sumA;
		sumB = temp_sumB;
		return true;
	}
	else
	{
		return false;
	}
}

void update_buckets(int &curr_cell, int &prev_gain)
{
	int new_gain;
	
	new_gain = cell_map[curr_cell].cell_gain;
	
	bucketA[prev_gain].erase(remove(bucketA[prev_gain].begin(),bucketA[prev_gain].end(),curr_cell),bucketA[prev_gain].end());
	
	if(bucketA[prev_gain].empty())
	{
		bucketA.erase(prev_gain);
	}
	bucketA[new_gain].push_back(curr_cell);
	
}

void update_gains(int &cell_to_move)
{
	int prev_gain;
	int new_gain;
	string curr_net;
	int curr_cell;
	
	for (i=0; i < cell_map[cell_to_move].net_list.size(); i++)
	{ 
		curr_net = cell_map[cell_to_move].net_list[i];
		if(old_partition == 0)
		{
			cell_map[cell_to_move].FS = net_map[curr_net].Asize;
			cell_map[cell_to_move].TE = net_map[curr_net].Bsize;
		}
		else if(old_partition == -1)
		{
			cell_map[cell_to_move].FS = net_map[curr_net].Bsize;
			cell_map[cell_to_move].TE = net_map[curr_net].Asize;
		}
		if(cell_map[cell_to_move].TE == 0)
		{
			for(j =0; j<net_map[curr_net].cell_list.size(); j++)
			{
				curr_cell = net_map[curr_net].cell_list[j];
				if(cell_map[curr_cell].lock_status == 0)
				{
					prev_gain = cell_map[curr_cell].cell_gain;
					cell_map[curr_cell].cell_gain = cell_map[curr_cell].cell_gain + 1;
					update_buckets(curr_cell, prev_gain);
				}
				
			}
		}
		
		else if(cell_map[cell_to_move].TE == 1)
		{
			for(j =0; j<net_map[curr_net].cell_list.size(); j++)
			{
				curr_cell = net_map[curr_net].cell_list[j];
				if(cell_map[cell_to_move].cell_partition == cell_map[curr_cell].cell_partition )
				{
					if(cell_map[curr_cell].lock_status == 0)
					{
						prev_gain = cell_map[curr_cell].cell_gain;
						cell_map[curr_cell].cell_gain = cell_map[curr_cell].cell_gain - 1;
						update_buckets(curr_cell, prev_gain);
						break;
					}
				}
			}
		}
		//making the move
		cell_map[cell_to_move].FS = cell_map[cell_to_move].FS - 1;
		cell_map[cell_to_move].TE = cell_map[cell_to_move].TE + 1;
		//after moving
			//updating Asize and Bsize and cell lists for current net
		if(old_partition == -1)
		{
			net_map[curr_net].Asize = cell_map[cell_to_move].TE;
			net_map[curr_net].Bsize = cell_map[cell_to_move].FS;
		}
		else if(old_partition == 0)
		{
			net_map[curr_net].Bsize = cell_map[cell_to_move].TE;
			net_map[curr_net].Asize = cell_map[cell_to_move].FS;
		}
		
		//checking criticality after the move
		if (cell_map[cell_to_move].FS == 0)//From partition of cell_to_move 'BEFORE the move'
		{
			for(j =0; j<net_map[curr_net].cell_list.size(); j++)
			{
				curr_cell = net_map[curr_net].cell_list[j];
				if(cell_map[curr_cell].lock_status == 0)
				{
					prev_gain = cell_map[curr_cell].cell_gain;
					cell_map[curr_cell].cell_gain = cell_map[curr_cell].cell_gain - 1;
					update_buckets(curr_cell, prev_gain);
				}
			}
		}
		else if (cell_map[cell_to_move].FS == 1)
		{
			for(j =0; j<net_map[curr_net].cell_list.size(); j++)
			{
				curr_cell = net_map[curr_net].cell_list[j];
				if(old_partition == cell_map[curr_cell].cell_partition)
				{
					if (cell_map[curr_cell].lock_status == 0)
					{
						prev_gain = cell_map[curr_cell].cell_gain;
						cell_map[curr_cell].cell_gain = cell_map[curr_cell].cell_gain + 1;
						update_buckets(curr_cell, prev_gain);
						break;
					}
				}
			}
		}	
	}	
}


//main function
int main (int argc, char* argv[]) 
{

  int partition = -1;
  int passes;
  int pass;
  string line;
  string curr_net;
  int curr_cell;
  int next_cell;
  int cell_to_move;
  int cell_to_check;
  
  vector <int> all_cells;
  vector <string> all_nets;
  vector <string> files;
  int node;
  //int temp_node;
  vector <string> temp_node;
  vector <string> str_node;
  vector <string> net_line;
  vector <string> inst_line;
	
  srand(time(NULL));
	
	cout<<"start : ";
	system("date");
	cout<<endl;
	
	if(argc < 2)
	{
	  cout<<"please specify the input file name\n";
	  return 0;
	}
	cout<<"Enter the number of passes you want to make: ";
	cin>>passes;
	//passes = 1;
	const char* arg = argv[1];
	ifstream inputfile(arg);
	if(inputfile.is_open())
	{
		while(getline (inputfile,line))
		{
		  if(line.find("design") != string::npos)
		  {
			  files = split(line, ' ');
		  }
		  
		}
		inputfile.close();
	}
  else cout<<"unable to open the inputfile";
	const char* file1 = files[2].c_str();
	const char* file2 = files[3].c_str();
		
  //cout<<"making "<<passes<<" passes on "<<files[2]<<" and "<<files[3]<<"\n";
  
  ifstream nodesfile (file1);
  //ofstream testfile	("testfile.csv");
  //system("date");
  ofstream resultfile ("Team09_output.txt");
  if (nodesfile.is_open())
  {
    while ( getline (nodesfile,line) )
    {
	str_node = split(line,' ');
	temp_node = split(str_node[0], '_');
	node = toint(temp_node[1]);
	cell_map[node].cell_type = str_node[1]; //updating cell type
	all_cells.push_back(node);
	
	/*****update cell size*****/
	if(cell_map[node].cell_type.compare("FDRE") == 0){cell_map[node].cell_size = 5;}
	else if(cell_map[node].cell_type.compare("LUT6") == 0){cell_map[node].cell_size = 7;}
	else if(cell_map[node].cell_type.compare("LUT5")== 0){cell_map[node].cell_size = 6;}
	else if(cell_map[node].cell_type.compare("LUT4") == 0){cell_map[node].cell_size = 5;}
	else if(cell_map[node].cell_type.compare("LUT3") == 0){cell_map[node].cell_size = 4;}
	else if(cell_map[node].cell_type.compare("LUT2") == 0){cell_map[node].cell_size = 3;}
	else if(cell_map[node].cell_type.compare("LUT1") == 0){cell_map[node].cell_size = 2;}
	else if(cell_map[node].cell_type.compare("CARRY8") == 0){cell_map[node].cell_size = 34;}
	else if(cell_map[node].cell_type.compare("DSP48E2") == 0){cell_map[node].cell_size = 429;}
	else if(cell_map[node].cell_type.compare("RAMB36E2") == 0){cell_map[node].cell_size = 379;}
	else if(cell_map[node].cell_type.compare("BUFGCE") == 0){cell_map[node].cell_size = 3;}
	else if(cell_map[node].cell_type.compare("IBUF") == 0){cell_map[node].cell_size = 2;}
	else if(cell_map[node].cell_type.compare("OBUF") == 0){cell_map[node].cell_size = 2;}
	
	/*****Random Partitioning*****/
	
	if((rand()%2) == 1)
	{
		partition = 0;
	}
	else
	{
		partition = -1;
	}
	if(partition == 0) 
	{
		if((sumB + cell_map[node].cell_size - sumA) <= 860 )
		{
			partition = ~partition;
	  partB:sumB = sumB + cell_map[node].cell_size;
			
		}
		else
		{
			goto partA;
		}
		//partitionB <<node[0]<<"\n";
	}
	else if (partition == -1) 
	{
		if((sumA + cell_map[node].cell_size - sumB) <= 860 )
		{
			partition = ~partition;
	  partA:sumA = sumA + cell_map[node].cell_size;

		}
		else
		{
			goto partB;
		}
			//partitionA <<node[0]<<"\n";
	}
	
	cell_map[node].cell_partition = partition;
	
	/*****Random Partitioning end****/
	
	
	cell_map[node].lock_status = 0;				//Lock status intialization (All cells unlocked)
	
    }
    nodesfile.close();
  }
  else cout << "Unable to open file"; 
	
  
  ifstream netsfile (file2);
  if(netsfile.is_open())
  {
	  while (getline(netsfile,line))
	  {
		  if(line.find("net net_") != string::npos)
		  {
			net_line = split(line, ' ');
			all_nets.push_back(net_line[1]);
			net_map[net_line[1]].Asize=0;
			net_map[net_line[1]].Bsize=0;
		  }
		  else if(line.find("net clk") != string::npos)
		  {
			net_line = split(line, ' ');
			all_nets.push_back(net_line[1]);
			net_map[net_line[1]].Asize=0;
			net_map[net_line[1]].Bsize=0;
		  }
		  else if (line.find("net controlSig") != string::npos)
		  {
			net_line = split(line, ' ');
			all_nets.push_back(net_line[1]);
			net_map[net_line[1]].Asize=0;
			net_map[net_line[1]].Bsize=0;
		  }
		  else if(line.find("endnet") != string::npos)
		  {
			  if((net_map[net_line[1]].Asize != 0) && (net_map[net_line[1]].Bsize != 0))
			  {
				  cutset++;
			  }
		  }
		  else
		  {
			 inst_line = split(line,' ');
			 inst_line = split(inst_line[0],'_');
			 node = toint(inst_line[1]);
			 cell_map[node].net_list.push_back(net_line[1]);
			 net_map[net_line[1]].cell_list.push_back(node);
			 
			 if(cell_map[node].cell_partition == 0)
				{
					//net_map[net_line[1]].cell_listA.push_back(node);
					net_map[net_line[1]].Asize++;
				}
			 else if (cell_map[node].cell_partition == -1)
				{
					//net_map[net_line[1]].cell_listB.push_back(node);
					net_map[net_line[1]].Bsize++;
				}
			 
		  }
		  
		  
	  }
	  netsfile.close();

  }
  else cout << "Unable to open file"; 
  
	startcut = cutset;
	mincut = cutset;
for(pass =1; pass<=passes; pass++)
{	
	
	if(pass>1)
	{
		cutset = mincut;
		sumA =sumA_mincut;
		sumB =sumB_mincut;
		//locked_cells.clear();
		
		for(i=0;i<locked_cells.size();i++)
		{
			curr_cell = locked_cells[i];
			cell_map[curr_cell].lock_status=0;
			if(lock_indx <= i)
			{
				cell_map[curr_cell].cell_partition = ~cell_map[curr_cell].cell_partition;
			}
		}
		
	  	for(i=0; i<all_nets.size(); i++)
		{
			curr_net = all_nets[i];
			net_map[curr_net].Asize = 0;
			net_map[curr_net].Bsize = 0;
			for(j=0;j<net_map[curr_net].cell_list.size();j++)
			{
				curr_cell = net_map[curr_net].cell_list[j];
				if(cell_map[curr_cell].cell_partition == 0)
				{
					//net_map[curr_net].cell_listA.push_back(curr_cell);
					net_map[curr_net].Asize++;
				}
				if(cell_map[curr_cell].cell_partition == -1)
				{
					//net_map[curr_net].cell_listB.push_back(curr_cell);
					net_map[curr_net].Bsize++;
				}
			}
		} 	
	}
/********Gain calculation*********/
 bucketA.clear();
 
 for(i=0;i<all_cells.size();i++)
 {
	curr_cell = all_cells[i];
	cell_map[curr_cell].cell_gain = 0;
	for (j=0; j < cell_map[curr_cell].net_list.size(); j++)
	{ 
		curr_net = cell_map[curr_cell].net_list[j];
		if(cell_map[curr_cell].cell_partition == 0)
		{
			cell_map[curr_cell].FS = net_map[curr_net].Asize;
			cell_map[curr_cell].TE = net_map[curr_net].Bsize;
		}
		else if(cell_map[curr_cell].cell_partition == -1)
		{
			cell_map[curr_cell].FS = net_map[curr_net].Bsize;
			cell_map[curr_cell].TE = net_map[curr_net].Asize;
		}
		if(cell_map[curr_cell].FS == 1)
		{
			cell_map[curr_cell].cell_gain = cell_map[curr_cell].cell_gain + 1;
		}
		if(cell_map[curr_cell].TE == 0)
		{
			cell_map[curr_cell].cell_gain = cell_map[curr_cell].cell_gain - 1;
		}
	}
	//----------------UPDATING BUCKETS-------------
	bucketA[cell_map[curr_cell].cell_gain].push_back(curr_cell);
	//----------------UPDATING BUCKETS END---------
 }

 
/************Gain calculation end******************/

locked_cells.clear();
indexA = bucketA.end();
indexA--;


LOOP:while(true)//(!deadA||!deadB)
  {
	if(bucketA.empty())
	{
		break;
	}
	else
	{
		cell_to_move = indexA->second.back();
		new_index = 0;
	}
	
	while(!balance_check(cell_to_move))
	{
		
		if(cell_to_move == indexA->second.front())
		{
			if(indexA == bucketA.begin())
			{
				break;
			}
			indexA--;
			new_index = 0;
			goto LOOP;
		}
		new_index++;
		cell_to_move = *(&indexA->second.back()-new_index);
		
	}
	cell_map[cell_to_move].lock_status = 1; 
	locked_cells.push_back(cell_to_move);
	new_cutset = cutset - cell_map[cell_to_move].cell_gain;
	
	indexA->second.erase(indexA->second.end()-(new_index+1));
	
	if(indexA->second.empty())
	{
		//cout<<"erasing last key"<<endl;
		bucketA.erase(indexA);
	}
	
		//cout << "Erased\n";
	cutset = new_cutset;
	old_partition = cell_map[cell_to_move].cell_partition;
	cell_map[cell_to_move].cell_partition = ~cell_map[cell_to_move].cell_partition;
	
	update_gains(cell_to_move); //update gains of the neigbours of cell_to_move
	
	
	if(new_cutset <= mincut)
	{
		mincut = new_cutset;
		sumA_mincut = sumA;
		sumB_mincut = sumB;
		lock_indx = locked_cells.size();
	}
	if(!bucketA.empty())
	{
		indexA = bucketA.end();
		indexA--;
	}
  }
}
//testfile.close();
end: float per = ((startcut-mincut)/(float)startcut)*100;
  resultfile<<"Starting cut: "<<startcut<<endl;
  resultfile<<"Final cut: "<<mincut<<endl;
  resultfile<<"percentage change: "<<per<<endl;
  resultfile.close();
  return 0;
}

