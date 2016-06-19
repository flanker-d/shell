#include <iostream>
#include <string.h>
//#include <boost/regex.hpp>
#include <regex>
#include <algorithm>
#include <vector>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>

void sig_child_handler(int sig)
{
  int status = 0;
  waitpid(-1, &status, 0);
}

std::vector<std::pair<std::string, std::vector<std::string> > > get_command_queue(std::string &cmd)
{
  std::regex regexp("([\\w\\s.-]+)");
  std::sregex_iterator it_begin = std::sregex_iterator(cmd.begin(), cmd.end(), regexp);
  std::sregex_iterator it_end = std::sregex_iterator();

  std::vector<std::pair<std::string, std::vector<std::string> > > cmd_queue;
  for (std::sregex_iterator it = it_begin; it != it_end; it++)
  {
    std::smatch match = *it;
    std::string lexem = match[1];
    std::regex regexp_lex("([\\w.-]+)");
    std::sregex_iterator it_lex_begin = std::sregex_iterator(lexem.begin(), lexem.end(), regexp_lex);
    std::sregex_iterator it_lex_end = std::sregex_iterator();
    int i = 0;
    std::string command;
    std::vector<std::string> params_vect;
    for(std::sregex_iterator it_lex = it_lex_begin; it_lex != it_lex_end; it_lex++)
    {
      std::smatch match_lex = *it_lex;
      std::string inner_lex = match_lex[1];
      if(i == 0)
        command = inner_lex;
      else
        params_vect.push_back(inner_lex);
      i++;
    }
    std::pair<std::string, std::vector<std::string> > tup = std::make_pair(command, params_vect);
    cmd_queue.push_back(tup);
  }
  return cmd_queue;
}

bool is_first_command(std::vector<std::pair<std::string, std::vector<std::string> > >::reverse_iterator &iter_begin, std::vector<std::pair<std::string, std::vector<std::string> > >::reverse_iterator &iter_cur)
{
  if(iter_begin == iter_cur)
    return true;
  else
    return false;
}

void exec_proc(std::string &command, std::vector<std::string> &params)
{
  char **arcv_char = new char *[params.size() + 1];
  arcv_char[0] = (char *) command.c_str();
  for (size_t i = 0; i < params.size(); i++)
  {
    std::string &str = params.at(i);
    arcv_char[i + 1] = (char *) str.c_str();
  }
  arcv_char[params.size() + 1] = NULL;
  execvp(arcv_char[0], arcv_char);
}

void run_processes(std::vector<std::pair<std::string, std::vector<std::string> > >::reverse_iterator &iter_begin, std::vector<std::pair<std::string, std::vector<std::string> > >::reverse_iterator &iter_cur, std::vector<std::pair<std::string, std::vector<std::string> > >::reverse_iterator &iter_end)
{
  if(iter_cur == iter_end)
    return;
  std::string command = (*iter_cur).first;
  std::vector<std::string> params = (*iter_cur).second;

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
      //int fd = open("result.out", O_RDWR | O_CREAT | O_TRUNC, 0666);
      close(STDOUT_FILENO);
      dup2(fd, STDOUT_FILENO);
      close(STDIN_FILENO);
      dup2(pfd[0], STDIN_FILENO);
      close(pfd[1]);
      close(pfd[0]);
      //std::cerr << "first: " << command << " " << params << std::endl;
      exec_proc(command, params);
    }
    else
    {
      close(STDIN_FILENO);
      dup2(pfd[0], STDIN_FILENO);
      close(pfd[1]);
      close(pfd[0]);
      //std::cerr << command << " " << params << std::endl;
      exec_proc(command, params);
    }
  }
}

void process_command(std::string &cmd)
{
  std::vector<std::pair<std::string, std::vector<std::string> > > cmd_queue = get_command_queue(cmd);

  std::vector<std::pair<std::string, std::vector<std::string> > >::reverse_iterator iter_begin =  cmd_queue.rbegin();
  std::vector<std::pair<std::string, std::vector<std::string> > >::reverse_iterator iter_end = cmd_queue.rend();
  std::vector<std::pair<std::string, std::vector<std::string> > >::reverse_iterator iter_cur = iter_begin;

  run_processes(iter_begin, iter_cur, iter_end);
}

int main(int argc, char *argv[])
{
  //std::string cmd("who | sort | uniq -c | sort -nk1");
  //std::string cmd("cat input.txt");
  //std::string cmd("cat input.txt | sort | head -n 1");
  //std::string cmd("cat input.txt | sort");

  std::string cmd;//("echo 1");
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
  return 0;
}
