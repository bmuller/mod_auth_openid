#include <iostream>
#include <sys/stat.h>
#include <unistd.h>
#include "moid.h"

using namespace opkele;
using namespace std;
using namespace modauthopenid;

void num_records(string db_location) {
  SessionManager *s = new SessionManager(db_location);
  cout << "There are " << s->num_records() << " records in the sessions table.\n";
  delete s;

  MoidConsumer *c = new MoidConsumer(db_location);
  cout << "There are " << c->num_records() << " records in the associations table.\n";
  delete c;

  NonceManager *n = new NonceManager(db_location);
  cout << "There are " << n->num_records() << " records in the nonces table.\n";
  delete n;
};

int main(int argc, char **argv) { 
  if(argc != 2) {
    cout << "usage: ./" << argv[0] << " <BDB database location>";
    return -1;
  }
  struct stat buffer ;
  if(access(argv[1], 0) == -1) {
    cout << "File \"" << argv[1] << "\" does not exist or cannot be read.\n";
    return -1;
  }
  num_records(string(argv[1]));
  return 0; 
}
