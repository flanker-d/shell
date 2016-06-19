#include <iostream>
#include <string.h>
#include <strstream>
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
  std::vector<std::string> tokens;
  std::stringstream ss;
  ss << cmd;
  std::copy(std::istream_iterator<std::string>(ss), std::istream_iterator<std::string>(), std::back_inserter(tokens));

  std::vector<std::vector<std::string>> cmdlist;
  std::vector<std::string>::iterator first = tokens.begin();
  std::vector<std::string>::iterator last = tokens.end();
  for (std::vector<std::string>::iterator it = first; it != last; ++it)
  {
    if (*it == "|")
    {
      cmdlist.emplace_back(first, it);
      first = it + 1;
    }
  }
  if (first != last)
  {
    cmdlist.emplace_back(first, last);
  }

  std::vector<std::pair<std::string, std::vector<std::string> > > cmd_queue;
  for(std::vector<std::vector<std::string> >::iterator it = cmdlist.begin(); it != cmdlist.end(); it++)
  {
    std::vector<std::string> vect = *it;
    std::string command;
    std::vector<std::string> params;
    for(std::vector<std::string>::iterator it_v = vect.begin(); it_v != vect.end(); it_v++)
    {
      if(it_v == vect.begin())
        command = *it_v;
      else
      {
        std::string param = *it_v;
        params.push_back(param);
      }
    }
    std::pair<std::string, std::vector<std::string> > pair = std::make_pair(command, params);
    cmd_queue.push_back(pair);
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
