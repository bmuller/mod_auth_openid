/*
Copyright (C) 2007 Butterfat, LLC (http://butterfat.net)

Permission is hereby granted, free of charge, to any person
obtaining a copy of this software and associated documentation
files (the "Software"), to deal in the Software without
restriction, including without limitation the rights to use,
copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following
conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

Created by bmuller <bmuller@butterfat.net>
*/

#include <iostream>
#include <sys/stat.h>
#include <unistd.h>
#include "mod_auth_openid.h"

using namespace std;
using namespace modauthopenid;

void print_databases(string db_location) {
  SessionManager s(db_location);
  s.print_table();
  s.close();

  MoidConsumer c(db_location, "blah", "balh");
  c.print_tables();
  c.close();
};

int main(int argc, char **argv) { 
  if(argc != 2) {
    cout << "usage: ./" << argv[0] << " <sqlite database location>";
    return -1;
  }
  struct stat buffer ;
  if(access(argv[1], 0) == -1) {
    cout << "File \"" << argv[1] << "\" does not exist or cannot be read.\n";
    return -1;
  }
  print_databases(string(argv[1]));
  return 0; 
}
