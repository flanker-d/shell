#include <iostream>
#include <string.h>
//#include <regex>
#include <algorithm>
//#include <tuple>
#include <vector>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>

//using vect_of_tup = std::vector<std::tuple<std::string, std::string>>;
using vect_of_tup = std::vector<std::pair<std::string, std::string>>;

void sig_child_handler(int sig)
{
  int status = 0;
  waitpid(-1, &status, 0);
  //printf("signal %d received\n", sig);
}

//vect_of_tup get_command_queue(std::string &cmd)
//{
//  std::regex regexp("(\\w+)\\s?(-\\w+)?");
//  auto it_begin = std::sregex_iterator(cmd.cbegin(), cmd.cend(), regexp);
//  auto it_end = std::sregex_iterator();

//  vect_of_tup cmd_queue;
//  for (auto it = it_begin; it != it_end; it++)
//  {
//    std::smatch match = *it;
//    std::string command = match[1];
//    std::string params = match[2];

//    auto tup = std::make_tuple(command, params);
//    cmd_queue.push_back(tup);
//    //std::cout << command << " " << params << std::endl;
//  }
//  return cmd_queue;
//}

void remove_chars_from_string(std::string &str, char* charsToRemove )
{
   for ( unsigned int i = 0; i < strlen(charsToRemove); ++i )
   {
      str.erase( remove(str.begin(), str.end(), charsToRemove[i]), str.end() );
   }
}

vect_of_tup get_command_queue_without_regexp(std::string &cmd)
{
  vect_of_tup cmd_queue;
  std::vector<std::string> cmd_vect;
  int start_pos = 0;
  int i = 0;
  for(i = 0; i < cmd.size(); i++)
  {
    if(cmd.c_str()[i] == '|')
    {
      std::string comm = cmd.substr(start_pos, i - start_pos);
      cmd_vect.push_back(comm);
      start_pos = i + 1;
    }
  }
  std::string comm = cmd.substr(start_pos, i - start_pos);
  cmd_vect.push_back(comm);


  for(auto it = cmd_vect.begin(); it != cmd_vect.end(); it++)
  {
    std::string full_command = *it;
    std::string command;
    std::string params;
    start_pos = 0;
    for(i = 0; i < full_command.size(); i++)
    {
      if(full_command.c_str()[i] == '-')
      {
        command = full_command.substr(start_pos, i - start_pos);
        start_pos = i;
      }
    }
    if(start_pos == 0)
      command = full_command;
    else
      params = full_command.substr(start_pos, i - start_pos);

    remove_chars_from_string(command, " ");
    remove_chars_from_string(params, " ");

    //auto tup = std::make_tuple(command, params);
    auto tup = std::make_pair(command, params);
    cmd_queue.push_back(tup);
  }

  return cmd_queue;
}

bool is_first_command(vect_of_tup::reverse_iterator &iter_begin, vect_of_tup::reverse_iterator &iter_cur)
{
  if(iter_begin == iter_cur)
    return true;
  else
    return false;
}

void run_processes(vect_of_tup::reverse_iterator &iter_begin, vect_of_tup::reverse_iterator &iter_cur, vect_of_tup::reverse_iterator &iter_end)
{
  if(iter_cur == iter_end)
    return;
  std::string command = std::get<0>(*iter_cur);
  std::string params = std::get<1>(*iter_cur);

  int pfd[2];
  pipe(pfd);
  if(!fork())
  {
    close(STDOUT_FILENO);
    dup2(pfd[1], STDOUT_FILENO);
    close(pfd[1]);
    close(pfd[0]);
    iter_cur++;
    run_processes(iter_begin, iter_cur, iter_end);
  }
  else
  {
    struct sigaction sigact;
    struct sigaction osigact;
    sigact.sa_handler = sig_child_handler;

    sigaction(SIGCHLD, &sigact, &osigact);

    if(is_first_command(iter_begin, iter_cur))
    {
      int fd = open("/home/box/result.out", O_RDWR | O_CREAT | O_TRUNC, 0666);
      close(STDOUT_FILENO);
      dup2(fd, STDOUT_FILENO);
      close(STDIN_FILENO);
      dup2(pfd[0], STDIN_FILENO);
      close(pfd[1]);
      close(pfd[0]);
      //std::cerr << "first: " << command << " " << params << std::endl;
      if(params.size() != 0)
        execlp(command.c_str(), command.c_str(), params.c_str(), NULL);
      else
        execlp(command.c_str(), command.c_str(), NULL);
    }
    else
    {
      close(STDIN_FILENO);
      dup2(pfd[0], STDIN_FILENO);
      close(pfd[1]);
      close(pfd[0]);
      //std::cerr << command << " " << params << std::endl;
      if(params.size() != 0)
        execlp(command.c_str(), command.c_str(), params.c_str(), NULL);
      else
        execlp(command.c_str(), command.c_str(), NULL);
    }
  }
}

void process_command(std::string &cmd)
{
  vect_of_tup cmd_queue = get_command_queue_without_regexp(cmd);

  vect_of_tup::reverse_iterator iter_begin =  cmd_queue.rbegin();
  vect_of_tup::reverse_iterator iter_end = cmd_queue.rend();
  vect_of_tup::reverse_iterator iter_cur = iter_begin;

  run_processes(iter_begin, iter_cur, iter_end);
}

int main(int argc, char *argv[])
{
  std::string cmd;//("who | sort | uniq -c | sort -nk1");
  //while(true)
  {
    std::getline(std::cin, cmd);
    if(!fork())
    {
      process_command(cmd);
    }
    else
    {
      struct sigaction sigact;
      struct sigaction osigact;
      sigact.sa_handler = sig_child_handler;

      sigaction(SIGCHLD, &sigact, &osigact);
    }
  }
  return 0;
}
