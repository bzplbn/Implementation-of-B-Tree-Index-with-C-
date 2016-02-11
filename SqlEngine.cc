/**
 * Copyright (C) 2008 by The Regents of the University of California
 * Redistribution of this file is permitted under the terms of the GNU
 * Public License (GPL).
 *
 * @author Junghoo "John" Cho <cho AT cs.ucla.edu>
 * @date 3/24/2008
 */

#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <algorithm>
#include "Bruinbase.h"
#include "SqlEngine.h"
#include "BTreeIndex.h"
#include "BTreeNode.h"
#include "RecordFile.h"
#include "PageFile.h"

using namespace std;

// external functions and variables for load file and sql command parsing 
extern FILE* sqlin;
int sqlparse(void);


RC SqlEngine::run(FILE* commandline)
{
  fprintf(stdout, "Bruinbase> ");

  // set the command line input and start parsing user input
  sqlin = commandline;
  sqlparse();  // sqlparse() is defined in SqlParser.tab.c generated from
               // SqlParser.y by bison (bison is GNU equivalent of yacc)

  return 0;
}

RC SqlEngine::select(int attr, const string& table, const vector<SelCond>& cond)
{
  RecordFile rf;   // RecordFile containing the table
  RecordId   rid;  // record cursor for table scanning

  RC     rc;
  int    key;     
  string value;
  int    count=0;
  int    diff;
  IndexCursor cursor, startCursor, endCursor, tempCursor;
  BTreeIndex bt;
  BTLeafNode l;
  PageFile pf;
  int i,j=0;
  int NE[30];
  int startKey, endKey;
  RecordId startRid, endRid;
  //bool index＝true;

  // open the table file

  
if ((rc = bt.open(table + ".idx", 'r')) < 0) {
    if ((rc = rf.open(table + ".tbl", 'r')) < 0) {
        fprintf(stderr, "Error: table %s does not exist\n", table.c_str());
        return rc;
    }
  // scan the table file from the beginning
  rid.pid = rid.sid = 0;
  count = 0;
  while (rid < rf.endRid()) {
    // read the tuple
    
    if ((rc = rf.read(rid, key, value)) < 0) {
      fprintf(stderr, "Error: while reading a tuple from table %s\n", table.c_str());
      goto exit_select;
    }

    // check the conditions on the tuple
    for (unsigned i = 0; i < cond.size(); i++) {
      // compute the difference between the tuple value and the condition value
      switch (cond[i].attr) {
      case 1:
	diff = key - atoi(cond[i].value);//atoi -- convert string to int
	break;
      case 2:
	diff = strcmp(value.c_str(), cond[i].value);
	break;
      }

      // skip the tuple if any condition is not met
      switch (cond[i].comp) {
      case SelCond::EQ:
	if (diff != 0) goto next_tuple;
	break;
      case SelCond::NE:
	if (diff == 0) goto next_tuple;
	break;
      case SelCond::GT:
	if (diff <= 0) goto next_tuple;
	break;
      case SelCond::LT:
	if (diff >= 0) goto next_tuple;
	break;
      case SelCond::GE:
	if (diff < 0) goto next_tuple;
	break;
      case SelCond::LE:
	if (diff > 0) goto next_tuple;
	break;
      }
    }

    // the condition is met for the tuple. 
    // increase matching tuple counter
    count++;

    // print the tuple 
    switch (attr) {
    case 1:  // SELECT key
      fprintf(stdout, "%d\n", key);
      break;
    case 2:  // SELECT value
      fprintf(stdout, "%s\n", value.c_str());
      break;
    case 3:  // SELECT *
      fprintf(stdout, "%d '%s'\n", key, value.c_str());
      break;
    }

    // move to the next tuple
    next_tuple:
    ++rid;
  }

  // print matching tuple count if "select count(*)"
  if (attr == 4) {
    fprintf(stdout, "%d\n", count);
  }
  rc = 0;

  // close the table file and return
  exit_select:
  rf.close();
  return rc;
 }
    else{
        if (attr == 2 || attr ==3) {
            if ((rc = rf.open(table + ".tbl", 'r')) < 0) {
                fprintf(stderr, "Error: table %s does not exist\n", table.c_str());
                return rc;
            }
        }
                startKey = 0;
                endKey = 2147483640;

                bt.locate(startKey, startCursor);
                bt.locate(endKey, endCursor);

                for (i = 0; i < cond.size(); i++) {
                
                switch (cond[i].comp) {
                    case SelCond::EQ:
                        rc = bt.locate(atoi(cond[i].value), cursor);
                        if (rc == 0) goto print_EQ;
                        
                        break;
                        
                    case SelCond::NE:
                        rc = bt.locate(atoi(cond[i].value), cursor);
                        bt.readForward(cursor, key, rid);
                        NE[j++]=key;
                        break;
                        
                    case SelCond::GT:
                        rc = bt.locate(atoi(cond[i].value)+1, cursor);
                        rc = bt.readForward(cursor, key, rid);
//                        cout<<"key"<<key<<endl;
                        cursor.eid  = cursor.eid - 1;
                        rc = bt.readForward(startCursor, startKey, startRid);
                        startCursor.eid  = startCursor.eid - 1;
                        if (key > startKey) {
                            startCursor = cursor;
                            startKey = key;
                        }
                        break;
                        
                    case SelCond::LT:
                        rc = bt.locate(atoi(cond[i].value)-0.01, cursor);
                        rc = bt.readForward(cursor, key, rid);
                        cursor.eid  = cursor.eid - 1;
                        rc = bt.readForward(endCursor, endKey, endRid);
                          endCursor.eid  = endCursor.eid - 1;
                        if (key < endKey) {
                            endCursor = cursor;
                            endKey = key;
                        }
                        break;
                        
                    case SelCond::GE:
                        rc = bt.locate(atoi(cond[i].value), cursor);
                        rc = bt.readForward(cursor, key, rid);
                        cursor.eid  = cursor.eid - 1;
                        rc = bt.readForward(startCursor, startKey, startRid);
                        startCursor.eid  = startCursor.eid - 1;
                        if (key > startKey) {
                            startCursor = cursor;
                            startKey = key;
                        }
                        break;
                        
                    case SelCond::LE:
                        rc = bt.locate(atoi(cond[i].value)+1, cursor);
                        rc = bt.readForward(cursor, key, rid);
                        if (rc == 0) cursor.eid  = cursor.eid - 1;
                        rc = bt.readForward(endCursor, endKey, endRid);
                        endCursor.eid  = endCursor.eid - 1;
                        if (key < endKey) {
                            endCursor = cursor;
                            endKey = key;
                        }
                        break;
                }
             }
        
               // count++;
        
                // print the tuple
            print:
            tempCursor = startCursor;
        
        
            do {
             rc = bt.readForward(tempCursor, key, rid);
             count++;
            for (i=0;i<j;i++){
                if (key == NE[i]){
                   count --;
                   goto next;
                }
                }
                //if (tempCursor.eid > 0) {
                switch (attr) {
                    case 1:  // SELECT key
                        fprintf(stdout, "%d\n", key);
                        break;
                    case 2:  // SELECT value
                        rf.read(rid, key, value);
                        fprintf(stdout, "%s\n", value.c_str());
                        break;
                    case 3:  // SELECT *
                        rf.read(rid, key, value);
                        fprintf(stdout, "%d '%s'\n", key, value.c_str());
                        break;
                }
                //}
            next:
           if (tempCursor.pid <= 0) break;
            
            
            }  while (tempCursor != endCursor) ;
        
            goto exit;

            print_EQ:
                switch (attr) {
                    case 1:  // SELECT key
                        bt.readForward(cursor, key, rid);
                        fprintf(stdout, "%d\n", key);
                        break;
                    case 2:  // SELECT value
                        bt.readForward(cursor, key, rid);
                        rf.read(rid, key, value);
                        fprintf(stdout, "%s\n", value.c_str());
                        break;
                    case 3:  // SELECT *
                        bt.readForward(cursor, key, rid);
                        rf.read(rid, key, value);
                        fprintf(stdout, "%d '%s'\n", key, value.c_str());
                        break;
                }
            count ++ ;
         
           exit:
            // print matching tuple count if "select count(*)"
            if (attr == 4) {
                fprintf(stdout, "%d\n", count);
            }
        
            rc = 0;
        
            bt.close();
            rf.close();
            return rc;
                
           }

}



RC SqlEngine::load(const string& table, const string& loadfile, bool index)
{
    RecordFile rf;   // RecordFile containing the table
    RecordId   rid;  // record cursor for table scanning
    BTreeIndex bt;
    
    RC     rc;
    int    key;
    string value;
    ifstream f(loadfile.c_str());
    string buf;
    
       if(!f.fail()){
        //cout << "into" << endl;
        if ((rc = rf.open(table + ".tbl", 'w')) < 0) {
            fprintf(stderr, "Error: cannot create table %s \n", table.c_str());
            return rc;
        }
        if (index == true)
            bt.open(table + ".idx", 'w');

           int parseLine = 0;
           while(!f.eof()) {
               
               getline(f, buf);
               
               if(buf == "") break;
               
               if((rc = parseLoadLine(buf, key, value)) < 0) {
                   fprintf(stderr, "Error while parsing from loadfile %s at line %i\n", loadfile.c_str(), parseLine);
                   break;
               }
               parseLine++;
               
               if((rc = rf.append(key, value, rid)) < 0) {
                   fprintf(stderr, "Error appending data to table %s\n", table.c_str());
                   break;
               }
               if (index == true)  rc = bt.insert(key, rid);

            }

           f.close();
            if((rc = bt.close())<0) return rc;
            if((rc = rf.close())<0) return rc;
        }
    
    
    return 0;
}



RC SqlEngine::parseLoadLine(const string& line, int& key, string& value)
{
    const char *s;
    char        c;
    string::size_type loc;
    
    // ignore beginning white spaces
    c = *(s = line.c_str());
    while (c == ' ' || c == '\t') { c = *++s; }

    // get the integer key value
    key = atoi(s);

    // look for comma
    s = strchr(s, ',');
    if (s == NULL) { return RC_INVALID_FILE_FORMAT; }

    // ignore white spaces
    do { c = *++s; } while (c == ' ' || c == '\t');
    
    // if there is nothing left, set the value to empty string
    if (c == 0) { 
        value.erase();
        return 0;
    }

    // is the value field delimited by ' or "?
    if (c == '\'' || c == '"') {
        s++;
    } else {
        c = '\n';
    }

    // get the value string
    value.assign(s);
    loc = value.find(c, 0);
    if (loc != string::npos) { value.erase(loc); }

    return 0;
}
