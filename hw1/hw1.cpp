#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "parser.h"
#include <vector>
#include <iostream>
#include <unistd.h>
#include <sys/fcntl.h>
#include <sys/wait.h>

#define BUFFER_SIZE 255
#define ARG_SIZE 255
#define MAX_INPUT_BUFFER 1048576

using namespace std;

class bundle
{

public:
    char *name;
    vector<char **> bundle_arguments;
    int numOfArguments;

    bundle(char *bundle_name)
    {
        name = bundle_name;
        numOfArguments = 0;
    }
};

std::vector<bundle>::iterator findBundleWithName(vector<bundle> bundle_vector, char *bundle_name)
{
    for (auto it = bundle_vector.begin(); it != bundle_vector.end(); it++)
        if (strcmp(bundle_name, it->name) == 0)
            return it;
}

void delBundleWithName(vector<bundle> bundle_vector, char *bundle_name)
{
    for (auto it = bundle_vector.begin(); it != bundle_vector.end(); it++)
        if (strcmp(bundle_name, it->name) == 0){
            bundle_vector.erase(it);
            return;
        }
            
}

int main(void)
{

    std::vector<bundle> all_bundles;
    int is_bundle_creation = 0;

    while (1)
    {

        
        

        parsed_input pinput;
        char buffer[BUFFER_SIZE];
        fgets(buffer, BUFFER_SIZE, stdin);

        parse(buffer, is_bundle_creation, &pinput);
        // cout<<pinput.command.type<<endl;

        if (is_bundle_creation == 1)
        {

            if (pinput.command.type == PROCESS_BUNDLE_STOP)
            {
                is_bundle_creation = 0;
            }

            else
            { // add commands to bundle
                bundle *last_added_bundle = &all_bundles.back();
                last_added_bundle->bundle_arguments.push_back(pinput.argv);
                last_added_bundle->numOfArguments++;
            }
        }

        else
        {

            if (pinput.command.type == QUIT)
                break;

            else if (pinput.command.type == PROCESS_BUNDLE_CREATE)
            {
                is_bundle_creation = 1;
                all_bundles.push_back(bundle(pinput.command.bundle_name));
            }

            else if (pinput.command.type == PROCESS_BUNDLE_EXECUTION)
            {
                if (pinput.command.bundle_count == 1)
                { // only 1 bundle execution
                    bundle execution_bundle = *findBundleWithName(all_bundles, pinput.command.bundles[0].name);

                    for (int i = 0; i < execution_bundle.numOfArguments; i++)
                    {
                        if (fork() == 0)
                        {

                            if (pinput.command.bundles[0].input != NULL)
                            {
                                int input_file_desc = open(pinput.command.bundles[0].input, O_RDONLY);
                                dup2(input_file_desc, 0);
                            }

                            if (pinput.command.bundles[0].output != NULL)
                            {
                                int output_file_desc = open(pinput.command.bundles[0].output, O_WRONLY | O_CREAT, 0655);
                                dup2(output_file_desc, 1);
                            }

                            execvp(execution_bundle.bundle_arguments[i][0], execution_bundle.bundle_arguments[i]);
                            exit(0);
                        }
                    }
                }

                else
                { // pipeline

                    int pp[pinput.command.bundle_count - 1][2];                 

                    for (int i = 0; i < pinput.command.bundle_count; i++)
                    {

                        pipe(pp[i]);
                        bundle execution_bundle = *findBundleWithName(all_bundles, pinput.command.bundles[i].name);
                        char input[MAX_INPUT_BUFFER] = "";

                        if (i != 0)
                        {
                            read(pp[i - 1][0], input, MAX_INPUT_BUFFER);
                        }

                        for (int j = 0; j < execution_bundle.numOfArguments; j++)
                        {
                            if (fork() == 0)
                            {

                                int inner_pipe[2];
                                pipe(inner_pipe);

                                if (i == 0)
                                {
                                    if (pinput.command.bundles[i].input != NULL)
                                    {
                                        int input_file_desc = open(pinput.command.bundles[i].input, O_RDONLY);
                                        dup2(input_file_desc, 0);
                                    }

                                    dup2(pp[i][1], 1);
                                }

                                else if (i == pinput.command.bundle_count - 1)
                                {
                                    if (pinput.command.bundles[i].output != NULL)
                                    {
                                        int output_file_desc = open(pinput.command.bundles[i].output, O_WRONLY | O_CREAT, 0655);
                                        dup2(output_file_desc, 1);
                                    }

                                    write(inner_pipe[1], input, strlen(input));
                                    dup2(inner_pipe[0], 0);
                                }

                                else
                                {

                                    write(inner_pipe[1], input, strlen(input));
                                    dup2(inner_pipe[0], 0);
                                    dup2(pp[i][1], 1);
                                }

                                close(inner_pipe[0]);
                                close(inner_pipe[1]);
                                execvp(execution_bundle.bundle_arguments[j][0], execution_bundle.bundle_arguments[j]);
                                exit(0);
                            }
                        }
                    }

                    for (int i = 0; i < pinput.command.bundle_count - 1; i++)
                    {
                        close(pp[i][0]);
                        close(pp[i][1]);
                    }
                }

                wait(NULL);
                
                for (int i = 0; i < pinput.command.bundle_count; i++)
                    delBundleWithName(all_bundles, pinput.command.bundles[i].name);
                    
                
            }
        }

        /*for (size_t i = 0; i < all_bundles.size(); i++)
        {
            cout<<all_bundles.at(i).name<<endl;
        }*/
    }

    return 0;
}



