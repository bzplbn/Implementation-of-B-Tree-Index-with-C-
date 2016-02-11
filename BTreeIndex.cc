/*
 * Copyright (C) 2008 by The Regents of the University of California
 * Redistribution of this file is permitted under the terms of the GNU
 * Public License (GPL).
 *
 * @author Junghoo "John" Cho <cho AT cs.ucla.edu>
 * @date 3/24/2008
 */
 
#include "BTreeIndex.h"
#include "BTreeNode.h"
#include "cstring"
#include "iostream"

using namespace std;

/*
 * BTreeIndex constructor
 */
BTreeIndex::BTreeIndex()
{
    rootPid = -1;
    memset(searchRoute, 0, sizeof(searchRoute));
    
}

bool operator != (const IndexCursor& r1, const IndexCursor& r2)
{
    return ((r1.pid != r2.pid) || (r1.eid != r2.eid));
}
//
//IndexCursor& operator++ (IndexCursor& cursor)
//{   BTLeafNode l;
//    PageFile pf;
//    l.read(cursor.pid, pf);
//    // if the end of a page is reached, move to the next page
//    if (++cursor.eid == -1) {
//        cursor.pid = ;
//        cursor.eid = 0;
//    }
//    
//    return cursor;
//}
/*
 * Open the index file in read or write mode.
 * Under 'w' mode, the index file should be created if it does not exist.
 * @param indexname[IN] the name of the index file
 * @param mode[IN] 'r' for read, 'w' for write
 * @return error code. 0 if no error
 */
RC BTreeIndex::open(const string& indexname, char mode)
{
    RC rc;
    rc = pf.open(indexname, mode);
    if (rc < 0) {
        return rc;
    }
    else return 0;
}

/*
 * Close the index file.
 * @return error code. 0 if no error
 */
RC BTreeIndex::close()
{
    RC rc;
    rc = pf.close();
    if (rc < 0) {
        return rc;
    }
    else return 0;
}

/*
 * Insert (key, RecordId) pair to the index.
 * @param key[IN] the key for the value inserted into the index
 * @param rid[IN] the RecordId for the record being inserted into the index
 * @return error code. 0 if no error
 */
RC BTreeIndex::insert(int key, const RecordId& rid)
{
       //pf.open(pf, char mode);
   int count=pf.endPid();
   BTLeafNode leafnode;
   BTNonLeafNode nonleafnode;
   BTLeafNode sibling;
   BTNonLeafNode nonsibling;
   int siblingKey;
   IndexCursor cursor;
   RC rc;
   int tmp = treeHeight;
   
    

   if (count==0)
   {
       
       leafnode.insert(key, rid);
       
      // pf.write(pf.endPid(), leafnode.getBuffer());
       rootPid=1;
       memcpy(buffer, &rootPid, sizeof(rootPid));
       pf.write(0, buffer);
       leafnode.write(pf.endPid(), pf);
       
       return 0;
       
   }

   else if(count<3)
   {
       
       leafnode.read(pf.endPid()-1, pf);
//       cout<< leafnode.getKeyCount() << "th"<<endl;
       rc=leafnode.insert(key,rid);
//       cout<<"leafnode insert"<<" "<<rc<<endl;
       if (rc==RC_NODE_FULL)
       {
           leafnode.insertAndSplit(key, rid, sibling, siblingKey);
           leafnode.setNextNodePtr(pf.endPid());
           leafnode.write(pf.endPid()-1, pf);
           sibling.write(pf.endPid(), pf);
           nonleafnode.initializeRoot(1, siblingKey, 2);
           rootPid=pf.endPid();
           memcpy(buffer, &rootPid, sizeof(rootPid));
           pf.write(0, buffer);
           nonleafnode.write(pf.endPid(), pf);
           
        
           return rc;
           
       }
       else { leafnode.write(pf.endPid()-1, pf);

          return 0;
       }
   }
    
   else
   {
       locate(key,cursor);
       leafnode.read(cursor.pid,pf);
       rc=leafnode.insert(key,rid);
       //treeHeight= sizeof(searchRoute);

       
       if (rc==RC_NODE_FULL)
       {
           PageId siblingNextPtr;
           leafnode.insertAndSplit(key, rid, sibling, siblingKey);
           siblingNextPtr = leafnode.getNextNodePtr();
           leafnode.setNextNodePtr(pf.endPid());
           leafnode.write(cursor.pid, pf);
           sibling.setNextNodePtr(siblingNextPtr);
           sibling.write(pf.endPid(), pf);
           
           
           int midKey=siblingKey;
           
           
           for (int i=1; i<treeHeight; i++) {
               BTNonLeafNode nonleafnode;
               BTNonLeafNode nonsibling;
               BTNonLeafNode rootnode;
               
               nonleafnode.read(searchRoute[i], pf);
               rc=nonleafnode.insert(midKey, pf.endPid()-1);
               if (rc==RC_NODE_FULL) {
                   nonleafnode.insertAndSplit(midKey, pf.endPid()-1, nonsibling, midKey);
                   nonleafnode.write(searchRoute[i], pf);
                   nonsibling.write(pf.endPid(), pf);
                   if (i==treeHeight-1) {
                       rootnode.initializeRoot(searchRoute[i], midKey,pf.endPid()-1);
                       rootPid=pf.endPid();
                       memcpy(buffer, &rootPid, sizeof(rootPid));
                       pf.write(0, buffer);
                       pf.write(pf.endPid(), rootnode.getBuffer());
                       tmp++;
                       
                   }
               }
               else {
                   nonleafnode.write(searchRoute[i], pf);
                   break;
               }
           }
           treeHeight = tmp;
           return rc;
       }
       else { leafnode.write(cursor.pid, pf);
           return 0;
       }
   }
}
/**
 * Run the standard B+Tree key search algorithm and identify the
 * leaf node where searchKey may exist. If an index entry with
 * searchKey exists in the leaf node, set IndexCursor to its location
 * (i.e., IndexCursor.pid = PageId of the leaf node, and
 * IndexCursor.eid = the searchKey index entry number.) and return 0.
 * If not, set IndexCursor.pid = PageId of the leaf node and
 * IndexCursor.eid = the index entry immediately after the largest
 * index key that is smaller than searchKey, and return the error
 * code RC_NO_SUCH_RECORD.
 * Using the returned "IndexCursor", you will have to call readForward()
 * to retrieve the actual (key, rid) pair from the index.
 * @param key[IN] the key to find
 * @param cursor[OUT] the cursor pointing to the index entry with
 *                    searchKey or immediately behind the largest key
 *                    smaller than searchKey.
 * @return 0 if searchKey is found. Othewise an error code
 */
RC BTreeIndex::locate(int searchKey, IndexCursor& cursor)
{
    PageId currentId;
    BTNonLeafNode nl;
    BTLeafNode l;
    RC rc;
    char checkBuffer[1024];
    int check, eid,i = 0;
    
    pf.read(0, buffer); //first read
    memcpy(&rootPid, buffer, sizeof(rootPid));
    currentId = rootPid;
    searchRoute[i++]= currentId;
    //    rc = pf.read(currentId, checkBuffer); //second read
    //
    //    if (rc < 0)
    //        return rc;
    //    else
    //        check =  checkBuffer[0];
    rc = nl.read(currentId, pf);  // third read;
    memcpy(checkBuffer, nl.getBuffer(), 1024);
    check = checkBuffer[0];
    
    while(!check){
        
        
        nl.locateChildPtr(searchKey, currentId);
        searchRoute[i++]= currentId;
        //             rc = pf.read(currentId, checkBuffer);
        //             if (rc < 0)
        //                return rc;
        //             else
        //                check =  checkBuffer[0];
        rc = nl.read(currentId, pf);  // third read;
        memcpy(checkBuffer, nl.getBuffer(), 1024);
        check = checkBuffer[0];
    }
    l.read(currentId, pf);
    rc = l.locate(searchKey, eid);
    cursor.pid = currentId;
    cursor.eid = eid;
    treeHeight = i;
    for (int j = 0; j < int(i/2); j++) {
        int temp;
        temp = searchRoute[j];
        searchRoute[j] = searchRoute[i-1-j];
        searchRoute[i-1-j]= temp;
    }
    return 0;


}

/*
 * Read the (key, rid) pair at the location specified by the index cursor,
 * and move foward the cursor to the next entry.
 * @param cursor[IN/OUT] the cursor pointing to an leaf-node index entry in the b+tree
 * @param key[OUT] the key stored at the index cursor location.
 * @param rid[OUT] the RecordId stored at the index cursor location.
 * @return error code. 0 if no error
 */
RC BTreeIndex::readForward(IndexCursor& cursor, int& key, RecordId& rid)
{
    PageId pid, eid;
    BTLeafNode l;
    RC rc;
    pid = cursor.pid;
    eid = cursor.eid;
    rc = l.read(pid, pf);
    if (rc < 0) return  rc;
   // else {
    rc = l.readEntry(eid, key, rid);
    if (cursor.eid == l.getKeyCount()){
        if(l.getNextNodePtr()==RC_INVALID_PID){
            cursor.pid = RC_INVALID_PID;
            cursor.eid = -1;
        }else{
            cursor.pid = l.getNextNodePtr();
            cursor.eid = 1;
        }
    }
    else{
        cursor.eid ++;
    }
    if(cursor.pid == RC_INVALID_PID)
            return RC_END_OF_TREE;
   
    return  rc;
}


