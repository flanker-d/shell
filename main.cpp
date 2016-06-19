#include <iostream>
#include <regex>
#include <tuple>
#include <vector>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>

using vect_of_tup = std::vector<std::tuple<std::string, std::string>>;

void sig_child_handler(int sig)
{
  int status = 0;
  waitpid(-1, &status, 0);
  //printf("signal %d received\n", sig);
}

vect_of_tup get_command_queue(std::string &cmd)
{
  std::regex regexp("(\\w+)\\s?(-\\w+)?");
  auto it_begin = std::sregex_iterator(cmd.cbegin(), cmd.cend(), regexp);
  auto it_end = std::sregex_iterator();

  vect_of_tup cmd_queue;
  for (auto it = it_begin; it != it_end; it++)
  {
    std::smatch match = *it;
    std::string command = match[1];
    std::string params = match[2];

    auto tup = std::make_tuple(command, params);
    cmd_queue.push_back(tup);
    //std::cout << command << " " << params << std::endl;
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
  vect_of_tup cmd_queue = get_command_queue(cmd);

  vect_of_tup::reverse_iterator iter_begin =  cmd_queue.rbegin();
  vect_of_tup::reverse_iterator iter_end = cmd_queue.rend();
  vect_of_tup::reverse_iterator iter_cur = iter_begin;

  run_processes(iter_begin, iter_cur, iter_end);
}

int main(int argc, char *argv[])
{
  std::string cmd;//("who | wc -l");
  while(true)
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
