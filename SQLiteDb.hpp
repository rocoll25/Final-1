#ifndef P_SQLITE_DB_WRAPPER_HPP
#define P_SQLITE_DB_WRAPPER_HPP

#include <string> // std::string 
#include <unistd.h> // read(), write(), pipe(), execvp(), close(), ssize_t
#include <cstdio> // perror(), printf()
#include <sys/select.h> // select(), FD_SET(), FD_ZERO(), timeval(?)


class SQLiteDb {
  int m_pipe[2];
  public:

  static const int ReadEnd, WriteEnd;

  SQLiteDb() {
    m_pipe[ReadEnd] = 0;
    m_pipe[WriteEnd] = 0;
  };

  SQLiteDb(std::string file) {
    m_pipe[ReadEnd] = 0;
    m_pipe[WriteEnd] = 0;
    this->connect(file);
  }

  ~SQLiteDb() {
    close(m_pipe[ReadEnd]);
    close(m_pipe[WriteEnd]);
  }

  int connect(std::string file) {
    if(m_pipe[ReadEnd] || m_pipe[WriteEnd]) {
      perror("SQLiteDb already connected");
      return -1; // exit(1);
    }
    //                  c r  w    p r  w
    int pipes[2][2] = { { 0, 0 }, { 0, 0 } };
    bool err = pipe(pipes[0]) || pipe(pipes[1]);
    if(err) {
      perror("pipe()");
      return -1; // exit(1);
    }

    int pid = fork();
    if(pid < 0) {
      perror("fork()");
      return -1; // exit(1);
    }

    if(pid == 0) { // child
      close(pipes[1][1]);
      close(pipes[0][0]);
      // NEED ERROR CHECKING!!
      // perror("njjj1");
      if(dup2(pipes[1][0], STDIN_FILENO) == -1) return -1; // shit.
      if(dup2(pipes[0][1], STDOUT_FILENO) == -1) return -1; // oh no this is definitely not good :/
      // perror("njjj2");
      char * const argv[] = { (char*)"sqlite3", (char*)file.data(), NULL };
      execvp("sqlite3",argv);
      printf("ERROR!");
      perror("ERROR!");
      exit(1);
    } 
    else { // parent
      close(pipes[1][0]);
      close(pipes[0][1]);
      m_pipe[ReadEnd] = pipes[0][0];
      m_pipe[WriteEnd] = pipes[1][1];
      usleep(1000); // just give the child some time to to get up and running first.
      return 0;
    }
  }

  int disconnect() {
    if((m_pipe[ReadEnd] | m_pipe[WriteEnd]) == 0)
      return 0;
    
    // else
    close(m_pipe[ReadEnd]);
    close(m_pipe[WriteEnd]); // will send EOF to child, causing child to exit. 
    m_pipe[ReadEnd] = 0;
    m_pipe[WriteEnd] = 0;
    return 0;
  }

  std::string execStatement(std::string statement, bool givesContent = true) {
    statement += '\n';
    write(m_pipe[WriteEnd], statement.data(), statement.length());
    // perror("njjj.1");
    std::string ret = "";
    if(givesContent) {
      unsigned char buf[1024]; // BUFSIZ
      fd_set set;
      FD_ZERO(&set);
      FD_SET(m_pipe[ReadEnd],&set);
      struct timeval tmo = { 1, 0 };  // 1 second
      // use select to see if there is any data
      while ( select(m_pipe[ReadEnd]+1,&set,NULL,NULL,&tmo) == 1 ) {
        ssize_t n = read(m_pipe[ReadEnd],buf,/*BUFSIZ*/1024 - 1);
        if ( n > 0 ) {
          buf[n] = '\0';
          ret += (const char*)buf;
        } 
        else {
          // read error, pipe closed by child exit
          ret = "ERROR!";
          break;
        }
        tmo.tv_sec = 1;  // reset timeout
      }
    }
    return ret;
  }
};

const int SQLiteDb::ReadEnd = 0;
const int SQLiteDb::WriteEnd = 1;

#endif