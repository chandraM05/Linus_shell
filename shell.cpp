#include<cstdio>
#include<vector>
#include<fcntl.h>
#include<ctype.h>
#include<fstream>
#include<signal.h>
#include<iostream>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<stdlib.h>
#include<signal.h>
#include<unistd.h>
#include<termios.h>
#include<sys/wait.h>
#include<sys/types.h>

using namespace std;


static int envset();
static void printvar();
void changeDirectory();
static void presentwd();
void redirect(char *cmd);
static void printhistory();
static void cleanup(int n);
static void getcmd(char* cmd);
void my_handle(int my_signal);
char *getenv(const char *name);
static void add_history(char *cmd);
static void bangop(int ,int ,int );
static int run(char* cmd, int input, int first, int last);


static int n = 0;
static char line[10240];
static int check_redirect;
static int iflag,oflag2,oflag1;
char inputfile[1024],outputfile[1024];


pid_t pid;
static int flag;
char bang[10240];
char prev[10240];
int command_pipe[2];
static char* args[1024];
static char* currentDirectory;


#define READ  0
#define WRITE 1


//Command execution 
static int command(int input, int first, int last)
{
	int connect[2];
 	int pid_status;
	pipe( connect );
	pid = fork();

	if (pid == 0) 
	{

		if(iflag==1)							//input output redirection
		{	
			int fdinp=open(inputfile,O_RDONLY);			//implementation
			dup2(fdinp,0);
			close(fdinp);
		}

		if((oflag1==1)||(oflag2==1))
		{

			if((oflag1==1)&&(oflag2==0))
			{
				int fdout=open(outputfile,O_WRONLY|O_TRUNC|O_CREAT,0777);
				dup2(fdout,1);
				close(fdout);
			}
			else if(oflag2==1)
			{
				int fdout=open(outputfile,O_WRONLY|O_APPEND|O_CREAT,0777);
				dup2(fdout,1);
				close(fdout);
			}

		}



		if (first == 1 && last == 0 && input == 0) 				//implementing pipe
			dup2( connect[WRITE], STDOUT_FILENO );

		else if (first == 0 && last == 0 && input != 0) 
		{
			dup2(input, STDIN_FILENO);
			dup2(connect[WRITE], STDOUT_FILENO);
		} 

		else 
			dup2( input, STDIN_FILENO );
 


		if (strcmp("history", args[0]) == 0)					//checking builtin commands
                	printhistory();

		else if(strcmp(args[0],"echo")==0)
			printvar();

		else if(strcmp(args[0],"pwd")==0)
			presentwd();

		else if(args[0][0]=='!')
			bangop(input,first,last);
		else
		{
			if(execvp( args[0], args) == -1)
			{
				cout<<args[0]<<" : "<<"command not found: -- refer man page for help"<<endl;
				exit(0);
			}
		}

	}

	else
		waitpid(pid,&pid_status,0);	
			
	if (last == 1)
		close(connect[READ]);

	if (input != 0) 
		close(input);

	close(connect[WRITE]);

	return connect[READ];

}
 

//Function to implement present working directory
static void presentwd()
{
	cout<<getcwd(currentDirectory, 1024)<<endl;
	exit(1);
}



//Skipping spaces in command
static char* skipsp(char* s)
{
	while (isspace(*s)) ++s;
	return s;
}



//signal handling
void my_handle(int my_signal)
{
	cout<<endl;
	signal(SIGINT,my_handle);
}


//Main
int main()
{

	currentDirectory = (char*) calloc(1024, sizeof(char));
	cout<<endl;
	cout<<"SIMPLE SHELL Implementation : Type 'exit' to end ."<<endl;
	cout<<"*****----------------------------------------------------------------------*****"<<endl;
	cout<<endl;

	signal(SIGINT,my_handle);

	while (1) 
	{
		cout<<"Chandra@my_shell:"<<getcwd(currentDirectory, 1024)<<":~$>";
 
		if (!fgets(line, 1024, stdin)) 
			return 0;
 
		int input = 0;
		int first = 1;
		char *cmd=line;
		cmd = skipsp(cmd);


		if(cmd[0]!='!')							//maintaining history
		{
		    if(flag==0)
		    {
			strcpy(prev,cmd);
			flag=1;
			add_history(prev);
		    }

		    else if(strcmp(cmd,prev)!=0)
		    {
			prev[0]='\0';
			strcpy(prev,cmd);
			add_history(prev);
		    }
		}


		char* next = strchr(cmd, '|');						//checking for pipes

		while (next != NULL)
		{
			int count1=0,count2=0;
			next[0]='\0';
			char *chk;
			char *dq=strchr(cmd,'"');					//checking metacharacters

			while(dq!=NULL)
			{
				count1++;
				chk=dq+1;
				dq=strchr(chk,'"');
			}

 			const char *quote="'";
			dq=strchr(cmd,*quote);

			while(dq!=NULL)
			{
				count2++;
				chk=dq+1;
				dq=strchr(chk,*quote);
			}

			if((count1%2==0)&&(count2%2==0))
			{
				input = run(cmd, input, first, 0);			//forwarding command for execution
				cmd = next + 1;
				next = strchr(next+1, '|'); 
				first = 0;
			}

			else
			{
				next[0]='|';
				next = strchr(next+1, '|'); 
			}
		}

		input = run(cmd, input, first, 1);

		cleanup(n);

		iflag=oflag2=oflag1=check_redirect=n=0;
	}
	return 0;

free(currentDirectory);
}


//Handling the commands
static int run(char* cmd, int input, int first, int last)				//maintains track of commands
{
	cmd = skipsp(cmd);
	char *in_id=strchr(cmd,'<');			
	char *out_id=strchr(cmd,'>');

	if((in_id==NULL)&&(out_id==NULL))
		getcmd(cmd);
    											//checking for redirection operators
	else 
	{
		redirect(cmd);
		if(check_redirect==1)
			getcmd(cmd);	
	}

	if ((args[0] != NULL) &&(strcmp(args[0],"export")!=0))
	{
		if (strcmp(args[0], "exit") == 0) 
		{
			cout<<"you have entered exit :Good Day"<<endl;
			exit(0);
		}

		if (strcmp("cd", args[0]) == 0)
		{
			if((first==1)&&(last==1))
			{
                		changeDirectory();
				return 1;
			}
		}

		n += 1;
		return command(input, first, last);
	}
	return 0;
}
 


//Splitting the command
static void getcmd(char* cmd)								//further splitting the command
{

	cmd = skipsp(cmd);
	char* next = strchr(cmd, ' ');
	int i = 0;
 	const char *quote="'";
	const char *isequ=strchr(cmd, '=');

	if(next != NULL) 
	{	

		next[0] = '\0';
		args[i] = cmd;
		++i;
		cmd = skipsp(next + 1);
		next = strchr(cmd, ' ');

	    while(next!=NULL)
	    {

		if((cmd[0]=='"')&&(strcmp(args[0],"echo")==0))
		{
			char* quotes=strrchr(cmd+1, '"');
			char* next1 = strchr(cmd, ' ');
			cmd++;

			while((next1!=NULL)&&(next1<quotes))
			{
				next1[0]='\0';
				args[i]=cmd;
				i++;
				cmd = skipsp(next1 + 1);
				next1=strchr(cmd,' ');
			}

			quotes[0]='\0';
			args[i]=cmd;
			++i;
			cmd = skipsp(quotes + 1);

		}
		else if(cmd[0]=='"')
		{
			char* quotes=strchr(cmd+1, '"');
			cmd++;
			quotes[0]='\0';
			args[i]=cmd;
			++i;
			cmd = skipsp(quotes + 1);
		}
		else if(cmd[0]==*quote)
		{
			char* quotes=strchr(cmd+1, *quote);
			cmd++;
			quotes[0]='\0';
			args[i]=cmd;
			++i;
			cmd = skipsp(quotes + 1);
		}
		else
		{
			next[0]='\0';
			args[i]=cmd;
			++i;
			cmd = skipsp(next + 1);
		}

		next = strchr(cmd, ' ');
            }
	}
		
	if (cmd[0] != '\0') 
	{	
		
		args[i] = cmd;
		next = strchr(cmd, '\n');
		if(next!=NULL)
		next[0] = '\0';
		++i; 	
		char *chk1=strchr(args[i-1],*quote);
		if((args[i-1][0]=='"')||(args[i-1][0]==*quote))	
		{	
			char *chk=strchr(args[i-1],'"');

			if(chk!=NULL)
			{
				args[i-1]=chk+1;
				chk=strrchr(chk+1,'"');
				chk[0]='\0';
			}
			else if(chk1!=NULL)
			{	
				args[i-1]=chk1+1;
				chk1=strchr(chk1+1,*quote);
				chk1[0]='\0';
			}
		}
	
	}

	args[i] = NULL;
	if(isequ!=NULL)
		envset();

}



//implementing redirection operator

void redirect(char *cmd)
{
	int flag=0;
	cmd = skipsp(cmd);							//checking for redirection operator
	char *in_id=strchr(cmd,'<');
	char *out_id=strchr(cmd,'>');


	cmd = skipsp(cmd);

	char* next1 = strchr(cmd, '<');
	char* next2 = strchr(cmd, '>');

	while (next1 != NULL)							//checking for valid input redirection operator
	{

		int count1=0,count2=0;
		next1[0]='\0';
		char *chk;
		char *dq=strchr(cmd,'"');

		while(dq!=NULL)
		{
			count1++;
			chk=dq+1;
			dq=strchr(chk,'"');
		}

 		const char *quote="'";
		dq=strchr(cmd,*quote);

		while(dq!=NULL)
		{
			count2++;
			chk=dq+1;
			dq=strchr(chk,*quote);
		}

		if((count1%2==0)&&(count2%2==0))
		{
			next1[0]='<';
			flag=1;
			break;
		}
		else
		{
			next1[0]='<';
			next1 = strchr(next1+1, '<'); 
		}
	}

	while (next2 != NULL)							//checking for valid output redirection operator
	{
		int count1=0,count2=0;
		next2[0]='\0';
		char *chk;
		char *dq=strchr(cmd,'"');

		while(dq!=NULL)
		{
			count1++;
			chk=dq+1;
			dq=strchr(chk,'"');
		}

 		const char *quote="'";
		dq=strchr(cmd,*quote);

		while(dq!=NULL)
		{
			count2++;
			chk=dq+1;
			dq=strchr(chk,*quote);
		}

		if((count1%2==0)&&(count2%2==0))
		{
			next2[0]='>';
			flag=1;
			break;
		}
		else
		{
			next2[0]='>';
			next2 = strchr(next2+1, '>'); 
		}
	}


   if(flag==0)
	check_redirect=1;

   else
   {
	in_id=next1;									//handling valid input output redirections
	out_id=next2;
	char *next;

	if(in_id!=NULL)
	{
		in_id[0]='\0';
		getcmd(cmd);
		cmd = skipsp(in_id + 1);
		next=strchr(cmd,' ');

		if(next==NULL)
		{
			next = strchr(cmd, '\n');
			if(next!=NULL)
			next[0] = '\0';
			inputfile[0]='\0';	
				strcpy(inputfile,cmd);
			iflag=1;
		}
		else
		{
			next[0]='\0';
			inputfile[0]='\0';	
				strcpy(inputfile,cmd);
			iflag=1;

			if(out_id==NULL)
				cmd = skipsp(next + 1);
			else
			{
				out_id[0]='\0';
				cmd=skipsp(out_id + 1);
			}

			next = strchr(cmd, '\n');
			if(next!=NULL)
			next[0] = '\0';
			outputfile[0]='\0';	
			strcpy(outputfile,cmd);
			oflag1=1;
			oflag2=0;
		}
	}

	else
	{
		out_id[0]='\0';
		getcmd(cmd);
		cmd = skipsp(out_id + 1);
		out_id=strchr(cmd,'>');
		if(out_id!=NULL)
		{
			oflag2=1;
			cmd=skipsp(out_id+1);
		}
		next = strchr(cmd, '\n');
		if(next!=NULL)
		next[0] = '\0';
		outputfile[0]='\0';	
		strcpy(outputfile,cmd);
		iflag=0;
		oflag1=1;
	}

   }

}


//Implementation of change directory command
void changeDirectory()
{
        if ((args[1] == NULL) ||(strcmp(args[1],"-")==0)||(strcmp(args[1],"--")==0))
	{
                chdir(getenv("HOME"));
        } 
	else if((strcmp(args[1],".")==0)||(strcmp(args[1],"./")==0))
	{}
	
	else 
	{
                if (chdir(args[1]) == -1) 
		{
                       cout<<args[1]<<" --no such directory"<<endl;
                }
        }

}


//Function to print environment variables
static void printvar()
{
	int i=1;

	while(args[i]!=NULL)			
	{
		char* dollar = strchr(args[i], '$');

		if(dollar!=NULL)									
		{
			char *var;
			var=dollar+1;

			if(var[0]=='0')								//cheching for special cases of echo
			{	
				char shell[20]="SHELL";
				char *slash =getenv(shell);
				slash=strrchr(slash,'/');
				cout<<(slash+1)<<" ";
			}

			else if(isdigit(var[0]))
			{
				var++;
				cout<<var<<" ";
			}

			else if(var[0]=='$')
				cout<<getppid()<<" ";

			else if(var[0]=='{')
			{
				var++;
				char *clos = strchr(var, '}');
				clos[0]='\0';
				args[i]=getenv(var);
				var=clos+1;
				strcat(args[i],var);
				cout<<args[i]<<" " ;
			}
			else
				cout<<getenv(var)<<" ";
		}

		else
			cout<<args[i]<<" ";
		i++;
	}

	cout<<endl;

exit(1);

}


//Function to set environment variables
static int envset()
{
	char *var;
	char *val;
	int i=0;

	while(args[i]!=NULL)
	{

		char *equal=strchr(args[i],'=');

		if(equal!=NULL)
		{
			if(isdigit(args[i][0]))
			{
				cout<<args[i]<<" : Not a valid identifier"<<endl;
				return 0;
			}

			equal[0]='\0';
			var=args[i];
			equal++;
			char *dq;

			if(equal[0]=='"')
			{
				equal++;
				dq=strrchr(equal+1,'"');
				dq[0]='\0';
			}

			if(equal[0]=='$')
			{
				equal++;
				val=getenv(equal);
			}
			else
				val=equal;
			break;
		}
		++i;
	}

	setenv(var,val,1);
	return 1;
}



//Function to implement history command
static void printhistory()
{

	int count=0;
	string buf;
	ifstream h_file;

	h_file.open("history_file.txt");						

	if(!h_file.is_open())
	{
		cout<<"Error in Reading the history_file.txt"<<endl;
		exit(0);
	}

	vector<pair<int,string> > his_vec;
	
	while(getline(h_file,buf))									//reading history file
	{

		his_vec.push_back(make_pair(++count,buf));
	}

	int size=his_vec.size();
	
	vector<pair<int,string> > ::iterator it=his_vec.begin();
							
	h_file.close();

	if((args[1]==NULL)&&(strcmp(args[0],"history")==0))
	{

		for(it=his_vec.begin();it !=his_vec.end();it++)
			cout<<(*it).first<<" "<<(*it).second.c_str()<<endl;

	}

	else if(strcmp(args[0],"history")==0)								//printing the commands 
	{

		int num=atoi(args[1]);

		for(it=his_vec.begin();it !=his_vec.end();it++)
		{
			if((*it).first>(size-num))
			cout<<(*it).first<<" "<<(*it).second.c_str()<<endl;
		}

	}

	exit (1);

}



//Adding commands to history file
static void add_history(char *cmd)
{
	ofstream h_file;

	h_file.open("history_file.txt",ios_base::app);
	if(!h_file.is_open())
	{
		cout<<"Error in opening the history file"<<endl;
		exit(1);
	}

	h_file<<cmd;									//adding commands to the history file
	h_file.close();

}



//Function to implement bang operator

static void bangop(int input, int first, int last)
{
	int count=0;
	string buf;
	ifstream h_file;

	h_file.open("history_file.txt");
	if(!h_file.is_open())
	{
		cout<<"Error in Reading the history_file.txt"<<endl;
		exit(0);
	}

	vector<pair<int,string> > his_vec;
	
	while(getline(h_file,buf))
	{

		his_vec.push_back(make_pair(++count,buf));
	}

	
	vector<pair<int,string> > ::iterator it=his_vec.begin();
							
	h_file.close();

	if(args[0][0]=='!')								//checking for cases of bang operator
	{
		if(args[0][1]=='!')
		{
			for(it=his_vec.begin();it !=his_vec.end();it++)
			{
				if((*it).first==count)
				{
					bang[0]='\0';
					strcpy(bang,(*it).second.c_str());
					cout<<bang<<endl;
				}
			}
		}
		else if(isdigit(args[0][1]))
		{
			char* numchar=args[0];
			numchar=numchar+1;
			int num=atoi(numchar);
			for(it=his_vec.begin();it !=his_vec.end();it++)
			{
				if((*it).first==num)
				{
					bang[0]='\0';
					strcpy(bang,(*it).second.c_str());
					cout<<bang<<endl;
					break;
				}
			}
		}
		else
		{
			for(it=his_vec.begin();it !=his_vec.end();it++)
			{
				if((*it).second[0]==args[0][1])
				{
					bang[0]='\0';
					strcpy(bang,(*it).second.c_str());

				}
			}
					cout<<bang<<endl;
		}
	}


	char *cmd;
	cmd = bang;									//executing the command selected by bang operator
	cmd = skipsp(cmd);
	strcat(cmd,"\n");

		if(cmd[0]!='!')
		{
			if(strcmp(cmd,prev)!=0)
			{
				prev[0]='\0';
				strcpy(prev,cmd);
				add_history(prev);
			}
		}

	char* next = strchr(cmd, '|');

	while (next != NULL)
	{ 	

		int count1=0,count2=0;
		next[0]='\0';
		char *chk;
		char *dq=strchr(cmd,'"');

			while(dq!=NULL)
			{
				count1++;
				chk=dq+1;
				dq=strchr(chk,'"');
			}

 		const char *quote="'";
		dq=strchr(cmd,*quote);

			while(dq!=NULL)
			{
				count2++;
				chk=dq+1;
				dq=strchr(chk,*quote);
			}

		if((count1%2==0)&&(count2%2==0))
		{
			input = run(cmd, input, first, 0);
			cmd = next + 1;
			next = strchr(next+1, '|'); 
			first = 0;
		}
		else
		{
			next[0]='|';
			next = strchr(next+1, '|'); 
		}
	}

	input = run(cmd, input, first, 1);

exit(1);

}

//deallocation
static void cleanup(int n)
{
	int i;
	for (i = 0; i < n; ++i) 
		wait(NULL); 
}
