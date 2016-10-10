/* CSci4061 F2016 Assignment 1
* login: sundi024
* date: 09/26/16
* name: Brent Higgins, Garrett Sundin, Yu Fang
* id: higgi295, sundi024, fangx174 */

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "util.h"

#define true     1
#define false    0

// Flags for controlling behavior during runtime
static int f_flag = false;
static int n_flag = false;
static int B_flag = false;

void show_error_message(char * lpszFileName)
{
	fprintf(stderr, "Usage: %s [options] [target] : only single target is allowed.\n", lpszFileName);
	fprintf(stderr, "-f FILE\t\tRead FILE as a makefile.\n");
	fprintf(stderr, "-h\t\tPrint this message and exit.\n");
	fprintf(stderr, "-n\t\tDon't actually execute commands, just print them.\n");
	fprintf(stderr, "-B\t\tDon't check files timestamps.\n");
	exit(0);
}
int needBuild(target_t * const targets, char * targetName, int nTargetCount)
{
	int i = 0;
	int rebuild = 0;

	int buildIndex = find_target(targetName, targets, nTargetCount);
	
	if (buildIndex != -1) //if it is a target
	{
		if(B_flag) //rebuild the target if true
			return 1;
		else //if there is a dependency that needs to be rebuilt return 1
		{
			for (i=0; i< targets[buildIndex].nDependencyCount; i++)
			{
				
				int compare = compare_modification_time(targetName, targets[buildIndex].szDependencies[i]);
				if( (compare != 0) && (compare != 1))
				{
					return 1;
				}
				//recursively call needBuild to access all dependent files
				// += allows us to tell if one or more sub-dependencies need to be rebuilt
				rebuild += needBuild(targets, targets[buildIndex].szDependencies[i], nTargetCount);
			} 
		}
	}
	//if sub-dependency needs to be rebuilt this file needs to be rebuilt
	if (rebuild >0)
		return 1;
	return 0; // do not rebuild
	
}
void dependencies(target_t * const targets, int targetNum, int nTargetCount)
{	
	int i = 0;
	int j = 0;

	for (j = 0; j < targets[targetNum].nDependencyCount; j++)
		{
			char * targetName = targets[targetNum].szDependencies[j];
			int buildIndex = find_target(targetName, targets, nTargetCount);
			int needRebuild = 0;
			needRebuild = needBuild(targets, targetName, nTargetCount);
			int need =is_file_exist(targetName); 
			// if you need the dependency and it is a target or if you need to rebuild the dependency
			if ( (need ==-1 && buildIndex != -1) || needRebuild == 1)
			{
				// Need to fork new process to build nonexistent dependency
				int status;
				targets[targetNum].pid = fork();
				if (targets[targetNum].pid == 1)
				{
					// There was an error, don't build anything
					perror("UNSUCCESSFUL FORK\n");
					exit(1);
				}
				else if (targets[targetNum].pid == 0){
					dependencies(targets, buildIndex, nTargetCount); //build dependencies
					exit(3);
				}
				else
				{
					wait(&status);
					if (WEXITSTATUS(status) != 0) //error in the child
						{
						printf("child exited with error code=%d\n", WEXITSTATUS(status));
						exit(-1);
						}
				}
			}
		}

		//check if n flag is true if true print command else run it
		if (n_flag)
		{
			printf("%s\n",targets[targetNum].szCommand);
			exit(0);
		}
		else
		{
			//printf("%s\n",targets[targetNum].szCommand); //shows does not double build
			execvp(targets[targetNum].prog_args[0], targets[targetNum].prog_args);
			//only get here if execvp fails
			exit(1);
		}
}

int main(int argc, char **argv)
{
	target_t targets[MAX_NODES]; //List of all the targets. Check structure target_t in util.h to understand what each target will contain.
	int nTargetCount = 0;

	// Declarations for getopt
	extern int optind;
	extern char * optarg;
	int ch;
	char * format = "f:hnB";

	// Variables you'll want to use
	char szMakefile[64] = "Makefile";
	char szTarget[64] = "";
	int i = 0;
	int j = 0;

	//init Targets
	for(i = 0; i < MAX_NODES; i++)
	{
		targets[i].pid = 0;
		targets[i].nDependencyCount = 0;
		strcpy(targets[i].szTarget, "");
		strcpy(targets[i].szCommand, "");
		targets[i].nStatus = FINISHED;
	}

	while((ch = getopt(argc, argv, format)) != -1)
	{
		switch(ch)
		{
			case 'f':
				//'f' (filename) flag: name of makefile
				//If no filename provided, assume filename is "makefile"
				f_flag = true;
				strcpy(szMakefile, strdup(optarg));
				break;
			case 'n':
				//'n' displays commands to be run, but doesn't run them
				n_flag = true;
				break;
			case 'B':
				//'B' always recompile (don't check timestamps for targets/input)
				//Set flag which can be used later to handle this case.
				B_flag = true;
				break;
			case 'h':
				//Case 'h' goes to default case, which prints help message
			default:
				//Default case prints errors/help
				show_error_message(argv[0]);
				exit(1);
		}
	}

	argc -= optind;
	argv += optind;

	if(argc > 1)
	{
		show_error_message(argv[0]);
		return EXIT_FAILURE;
	}

	/* Parse graph file or die */
	if((nTargetCount = parse(szMakefile, targets)) == -1)
	{
		return EXIT_FAILURE;
	}

	//Setting Targetname
	//if target is not set, set it to default (first target from makefile)
	if(argc == 1)
	{
		strcpy(szTarget, argv[0]);
	}
	else
	{
		strcpy(szTarget, targets[0].szTarget);
	}

	//find the index of the target we are looking for
	int targetIndex = find_target(szTarget, targets, nTargetCount);
	//find dependencies and build
	dependencies(targets, targetIndex, nTargetCount);

	return EXIT_SUCCESS;
}
