#include <iostream>
#include <string.h>
#include <boost/regex.hpp>
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
//using vect_of_tup = std::vector<std::pair<std::string, std::string> >;

void sig_child_handler(int sig)
{
  int status = 0;
  waitpid(-1, &status, 0);
  //printf("signal %d received\n", sig);
}

void remove_chars_from_string(std::string &str, char* charsToRemove )
{
  for ( unsigned int i = 0; i < strlen(charsToRemove); ++i )
  {
    str.erase( remove(str.begin(), str.end(), charsToRemove[i]), str.end() );
  }
}

std::vector<std::pair<std::string, std::string> > get_command_queue(std::string &cmd)
{
  boost::regex regexp("([\\w.\\s]+)\\s?(-\\w+\\s\\d)?");
  boost::sregex_iterator it_begin = boost::sregex_iterator(cmd.begin(), cmd.end(), regexp);
  boost::sregex_iterator it_end = boost::sregex_iterator();

  std::vector<std::pair<std::string, std::string> > cmd_queue;
  for (boost::sregex_iterator it = it_begin; it != it_end; it++)
  {
    boost::smatch match = *it;
    std::string command = match[1];
    std::string params = match[2];

    boost::regex regexp_cat("(\\w+)(\\s)(\\w+\\.?\\w+)");
    boost::sregex_iterator it_cat_begin = boost::sregex_iterator(command.begin(), command.end(), regexp_cat);
    boost::sregex_iterator it_cat_end = boost::sregex_iterator();
    for (boost::sregex_iterator it_cat = it_cat_begin; it_cat != it_cat_end; it_cat++)
    {
      boost::smatch match_cat = *it_cat;
      std::string command_cat = match_cat[1];
      std::string params_cat = match_cat[3];
      command = command_cat;
      params = params_cat;
    }

    remove_chars_from_string(command, (char *) " ");

    std::pair<std::string, std::string> tup = std::make_pair(command, params);
    cmd_queue.push_back(tup);
    //std::cout << command << " " << params << std::endl;
  }
  return cmd_queue;
}

std::vector<std::pair<std::string, std::string> > get_command_queue_without_regexp(std::string &cmd)
{
  std::vector<std::pair<std::string, std::string> > cmd_queue;
  std::vector<std::string> cmd_vect;
  int start_pos = 0;
  int i = 0;
  for(i = 0; i < (int) cmd.size(); i++)
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


  for(std::vector<std::string>::iterator it = cmd_vect.begin(); it != cmd_vect.end(); it++)
  {
    std::string full_command = *it;
    std::string command;
    std::string params;
    start_pos = 0;
    for(i = 0; i < (int) full_command.size(); i++)
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

    remove_chars_from_string(command, (char *) " ");
    remove_chars_from_string(params, (char *) " ");

    //auto tup = std::make_tuple(command, params);
    std::pair<std::string, std::string> tup = std::make_pair(command, params);
    cmd_queue.push_back(tup);
  }

  return cmd_queue;
}

bool is_first_command(std::vector<std::pair<std::string, std::string> >::reverse_iterator &iter_begin, std::vector<std::pair<std::string, std::string> >::reverse_iterator &iter_cur)
{
  if(iter_begin == iter_cur)
    return true;
  else
    return false;
}

void exec_proc(std::string &command, std::string &params)
{
  boost::regex regexp("[-.\\w]+");
  boost::sregex_iterator it_begin = boost::sregex_iterator(params.begin(), params.end(), regexp);
  boost::sregex_iterator it_end = boost::sregex_iterator();

  std::vector<std::string> par_vect;
  par_vect.push_back(command);
  for (boost::sregex_iterator it = it_begin; it != it_end; it++)
  {
    boost::smatch match = *it;
    par_vect.push_back(match.str());
  }

  char **arcv_char = new char *[par_vect.size() + 1];
  for (size_t i = 0; i < par_vect.size(); i++)
  {
    std::string &str = par_vect.at(i);
    arcv_char[i] = (char *) str.c_str();
  }
  arcv_char[par_vect.size()] = NULL;
  execvp(arcv_char[0], arcv_char);
}

void run_processes(std::vector<std::pair<std::string, std::string> >::reverse_iterator &iter_begin, std::vector<std::pair<std::string, std::string> >::reverse_iterator &iter_cur, std::vector<std::pair<std::string, std::string> >::reverse_iterator &iter_end)
{
  if(iter_cur == iter_end)
    return;
  std::string command = (*iter_cur).first;
  std::string params = (*iter_cur).second;

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
  std::vector<std::pair<std::string, std::string> > cmd_queue = get_command_queue(cmd);

  std::vector<std::pair<std::string, std::string> >::reverse_iterator iter_begin =  cmd_queue.rbegin();
  std::vector<std::pair<std::string, std::string> >::reverse_iterator iter_end = cmd_queue.rend();
  std::vector<std::pair<std::string, std::string> >::reverse_iterator iter_cur = iter_begin;

  run_processes(iter_begin, iter_cur, iter_end);
}

int main(int argc, char *argv[])
{
  //std::string cmd("who | sort | uniq -c | sort -nk1");
  //std::string cmd("cat input.txt");
  std::string cmd;//("cat input.txt | sort | head -n 1");
  //std::string cmd("cat input.txt | sort");
  //while(true)
  //  {
  std::getline(std::cin, cmd);
  //    if(!fork())
  //    {
  process_command(cmd);
  //    }
  //    else
  //    {
  //      struct sigaction sigact;
  //      struct sigaction osigact;
  //      sigact.sa_handler = sig_child_handler;

  //      sigaction(SIGCHLD, &sigact, &osigact);
  //    }
  //  }
  return 0;
}
