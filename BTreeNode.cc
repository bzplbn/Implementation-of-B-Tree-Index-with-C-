#include "BTreeNode.h"
#include "iostream"
using namespace std;

/*
 initialization
 buffer[0] is a flag for leaf node or non leaf node
 buffe[1-4] is the key count
*/
BTLeafNode::BTLeafNode()
{
    buffer[0]=1;
//    int NO=0;
//    memcpy(buffer+1,&NO,sizeof(BTLeafNode));
    memset(buffer+1, 0, sizeof(BTLeafNode)-1);
    
}

/*
 * Set the buffer
 * @param newBuffer[IN]: the input buffer value
 * @param size [IN]: size of newBuffer
 */
void BTLeafNode:: setBuffer(char *newBuffer, int size)

{
    int count;
    count=getKeyCount();
    
    memmove(buffer+5,newBuffer,size);
    count+=size/(sizeof(int)*3);
    memmove(buffer+1,&count,sizeof(int));
    
}

/*
 * Get the buffer
 */
char* BTLeafNode::getBuffer(){
    return buffer;
}


/*
 * Read the content of the node from the page pid in the PageFile pf.
 * @param pid[IN] the PageId to read
 * @param pf[IN] PageFile to read from
 * @return 0 if successful. Return an error code if there is an error.
 */

RC BTLeafNode::read(PageId pid, const PageFile& pf)
{
    
    RC rc;
    rc=pf.read(pid,buffer);
    if (rc<0) return rc;
    
    else return 0;
    
    //return 0;
}

/*
 * Write the content of the node to the page pid in the PageFile pf.
 * @param pid[IN] the PageId to write to
 * @param pf[IN] PageFile to write to
 * @return 0 if successful. Return an error code if there is an error.
 */

RC BTLeafNode::write(PageId pid, PageFile& pf)
{   RC rc;
    rc=pf.write(pid, buffer);
    if (rc<0) return rc;
    else return 0;
    //return 0;
}

/*
 * Return the number of keys stored in the node.
 * @return the number of keys in the node
 */
int BTLeafNode::getKeyCount()
{   int number;
    memmove(&number,buffer+1,4);
    return number; }

/*
 * Insert a (key, rid) pair to the node.
 * @param key[IN] the key to insert
 * @param rid[IN] the RecordId to insert
 * @return 0 if successful. Return an error code if the node is full.
 */
RC BTLeafNode::insert(int key, const RecordId& rid)
{
    char *currentRidCursor = buffer + 5;
    char *currentKeyCursor = currentRidCursor + sizeof(RecordId);
    int currentKey, count, tmp;
    
    memmove(&currentKey, currentKeyCursor, sizeof(int));
    count = getKeyCount();
    tmp = count;
    
    if (getKeyCount() == keyMax)
        return RC_NODE_FULL;
    
    else{
        if (getKeyCount() == 0){
            memmove(currentRidCursor, &rid, sizeof(RecordId));
            memmove(currentKeyCursor, &key, sizeof(int));
            tmp++;
        }
        
        else{
            for (int i = 0; i < count; i++) {
                memmove(&currentKey, currentKeyCursor, sizeof(int));
                if (key < currentKey) {
                    memmove(currentRidCursor + sizeof(int) + sizeof(RecordId), currentRidCursor, (count - i) * ( sizeof(int)+ sizeof(RecordId)));
                    memmove(currentRidCursor, &rid, sizeof(RecordId));
                    memmove(currentKeyCursor, &key, sizeof(int));
                    tmp++;
                    break;
                }
                else{
                    currentRidCursor += sizeof(int) + sizeof(RecordId);
                    currentKeyCursor += sizeof(int) + sizeof(RecordId);
                    if (i == count - 1) {
                        memmove(currentRidCursor, &rid, sizeof(RecordId));
                        memmove(currentKeyCursor, &key, sizeof(int));
                        tmp++;
                    }
                }
                
            }
        }
        count = tmp;
        memmove(buffer + 1, &count, sizeof(int));
        return 0;
    }

}

/*
 * Insert the (key, rid) pair to the node
 * and split the node half and half with sibling.
 * The first key of the sibling node is returned in siblingKey.
 * @param key[IN] the key to insert.
 * @param rid[IN] the RecordId to insert.
 * @param sibling[IN] the sibling node to split with. This node MUST be EMPTY when this function is called.
 * @param siblingKey[OUT] the first key in the sibling node after split.
 * @return 0 if successful. Return an error code if there is an error.
 */
RC BTLeafNode::insertAndSplit(int key, const RecordId& rid,
                              BTLeafNode& sibling, int& siblingKey)
{

    char *currentRidCursor = buffer + 5;
    char *currentKeyCursor = currentRidCursor + sizeof(RecordId);
    int currentKey, count, tmp;
    
    memmove(&currentKey, currentKeyCursor, sizeof(int));
    count = getKeyCount();
    tmp = count;
    

        if (getKeyCount() == 0){
            memmove(currentRidCursor, &rid, sizeof(RecordId));
            memmove(currentKeyCursor, &key, sizeof(int));
            tmp++;
            return 0;
        }
        
        else{
            for (int i = 0; i < count; i++) {
                memmove(&currentKey, currentKeyCursor, sizeof(int));
                if (currentKey > key) {
                    memmove(currentRidCursor + sizeof(int) + sizeof(RecordId), currentRidCursor, (count - i) * ( sizeof(int)+ sizeof(RecordId)));
                    memmove(currentRidCursor, &rid, sizeof(RecordId));
                    memmove(currentKeyCursor, &key, sizeof(int));
                    tmp++;
                    break;
                }
                else{
                    currentRidCursor += sizeof(int) + sizeof(RecordId);
                    currentKeyCursor += sizeof(int) + sizeof(RecordId);
                    if (i == count - 1) {
                        memmove(currentRidCursor, &rid, sizeof(RecordId));
                        memmove(currentKeyCursor, &key, sizeof(int));
                        tmp++;
                    }
                }
                
            }
        count = tmp;
        int count1=int(count)/2 + 1;
        memmove(buffer+1,&count1,sizeof(count1));
        int count2=count-count1;

        memmove(&siblingKey,buffer+5+count1*(sizeof(key)+sizeof(rid))+sizeof(rid),sizeof(siblingKey));
        sibling.setBuffer(buffer+5+(count1)*(sizeof(key)+sizeof(rid)), count2*(sizeof(key)+sizeof(rid)));
        memset(buffer+5+(count1) * (sizeof(key)+sizeof(rid)),0,count2 * (sizeof(key)+sizeof(rid)));
        return 0;
        
    }
    
    
}

/**
 * If searchKey exists in the node, set eid to the index entry
 * with searchKey and return 0. If not, set eid to the index entry
 * immediately after the largest index key that is smaller than searchKey,
 * and return the error code RC_NO_SUCH_RECORD.
 * Remember that keys inside a B+tree node are always kept sorted.
 * @param searchKey[IN] the key to search for.
 * @param eid[OUT] the index entry number with searchKey or immediately
 behind the largest key smaller than searchKey.
 * @return 0 if searchKey is found. Otherwise return an error code.
 */
RC BTLeafNode::locate(int searchKey, int& eid)
{
    int count=getKeyCount();
    int key1;
    int i;
    for ( i = 1; i <= count; i++)
    {
        memmove(&key1, buffer+5+sizeof(RecordId)*i+sizeof(searchKey)*(i-1), sizeof(key1));
        //cout<<"key1: "<<key1<<"searchKey: "<<searchKey<<endl;
        if (key1>=searchKey)
        {
            if (key1==searchKey)
            {
                eid=i;
                return 0;
            }
            else
            {
                eid=i;
                return RC_NO_SUCH_RECORD;
            }
            break;
        }
       
    }
    if (i==count+1) {
         eid=count+1;
    }
    return 0;
}

/*
 * Read the (key, rid) pair from the eid entry.
 * @param eid[IN] the entry number to read the (key, rid) pair from
 * @param key[OUT] the key from the entry
 * @param rid[OUT] the RecordId from the entry
 * @return 0 if successful. Return an error code if there is an error.
 */
RC BTLeafNode::readEntry(int eid, int& key, RecordId& rid)
{
    if (eid<=getKeyCount()) {
        memmove(&key,buffer+5+sizeof(RecordId)*eid+sizeof(key)*(eid-1),sizeof(key));
        memmove(&rid,buffer+5+sizeof(RecordId)*(eid-1)+sizeof(key)*(eid-1),sizeof(RecordId));
        return 0;
    }
    else{ return -1;}
    
    // return 0; 
}
/*
 * Return the pid of the next slibling node.
 * @return the PageId of the next sibling node 
 */
PageId BTLeafNode::getNextNodePtr()
{      
    
    PageId pid;
    memmove(&pid,buffer+sizeof(buffer)-sizeof(pid),sizeof(pid));
    
    if (pid == 0) return RC_INVALID_PID;
    return pid;
    
    //return 0; 
}

/*
 * Set the pid of the next slibling node.
 * @param pid[IN] the PageId of the next sibling node 
 * @return 0 if successful. Return an error code if there is an error.
 */
RC BTLeafNode::setNextNodePtr(PageId pid)
{
    if (pid<0) return -1;
    else {
        memmove(buffer+sizeof(buffer)-sizeof(pid),&pid,sizeof(pid));
        return 0;
    }
}




//*********************************************** Non-leaf Node *******************************************


/*
 *initialization
 *buffer[0] is a flag for leaf node or non leaf node
 *buffe[1-4] is the key count
 */
BTNonLeafNode::BTNonLeafNode()
{
    memset(buffer, 0, sizeof(BTNonLeafNode) );

}


/*
 * Get the buffer
 */
char* BTNonLeafNode::getBuffer(){
    return buffer;
}

/*
 * Set the buffer
 * @param newBuffer[IN]: the input buffer value
 * @param size [IN]: size of newBuffer
 */
void BTNonLeafNode::setBuffer(char * newBuffer, int size){
    int count;
    count = getKeyCount();
    memmove(buffer+ 5 + 3 * sizeof(int), newBuffer, size);
    count += size/(2 * sizeof(int));
    memmove(buffer+1, &count, sizeof(int));
}
/*
 * Read the content of the node from the page pid in the PageFile pf.
 * @param pid[IN] the PageId to read
 * @param pf[IN] PageFile to read from
 * @return 0 if successful. Return an error code if there is an error.
 */
RC BTNonLeafNode::read(PageId pid, const PageFile& pf)
{
    RC rc;
    rc = pf.read(pid, buffer);
    if (rc < 0) return rc;
    else return 0;
}
    
/*
 * Write the content of the node to the page pid in the PageFile pf.
 * @param pid[IN] the PageId to write to
 * @param pf[IN] PageFile to write to
 * @return 0 if successful. Return an error code if there is an error.
 */
RC BTNonLeafNode::write(PageId pid, PageFile& pf)
{
    RC rc;
    rc = pf.write(pid, buffer);
    if (rc < 0) return rc;
    else return 0;
}

/*
 * Return the number of keys stored in the node.
 * @return the number of keys in the node
 */
int BTNonLeafNode::getKeyCount()
{
    int count;
    memmove(&count, buffer+1, sizeof(int));
    return count;
}


/*
 * Insert a (key, pid) pair to the node.
 * @param key[IN] the key to insert
 * @param pid[IN] the PageId to insert
 * @return 0 if successful. Return an error code if the node is full.
 */
RC BTNonLeafNode::insert(int key, PageId pid)
{
    char *currentKeyCursor = buffer + 5 + 3 * sizeof(int);
    char *currentPidCursor = currentKeyCursor + sizeof(int);
    int currentKey, count, tmp;
    
    memcpy(&currentKey, currentKeyCursor, sizeof(int));
    count = getKeyCount();
    tmp = count;
    
    if (getKeyCount() == keyMax)
        return RC_NODE_FULL;
    
    else{
        if (getKeyCount() == 0){
            memmove(currentKeyCursor, &key, sizeof(int));
            memmove(currentPidCursor, &pid, sizeof(int));
            tmp++;;
        }
        
        else{
            currentKeyCursor = buffer + 5 + sizeof(int);                                 //skip first pid
            currentPidCursor = currentKeyCursor + sizeof(int);
            for (int i = 0; i < count; i++) {
            memmove(&currentKey, currentKeyCursor, sizeof(int));
            if (key < currentKey) {
            memmove(currentKeyCursor + 2 * sizeof(int), currentKeyCursor, (count - i) * 2 * sizeof(int) );
            memmove(currentKeyCursor, &key, sizeof(int));
            memmove(currentPidCursor, &pid, sizeof(int));
            tmp++;
            break;
            }
            else{
                currentKeyCursor += 2 * sizeof(int);
                currentPidCursor += 2 * sizeof(int);
                if (i == count - 1) {
                    memmove(currentKeyCursor, &key, sizeof(int));
                    memmove(currentPidCursor, &pid, sizeof(int));
                    tmp++;
                }
            }

          }
        }
        count = tmp;
        memcpy(buffer + 1, &count, sizeof(int));
        return 0;
    }
}



/*
 * Insert the (key, pid) pair to the node
 * and split the node half and half with sibling.
 * The middle key after the split is returned in midKey.
 * @param key[IN] the key to insert
 * @param pid[IN] the PageId to insert
 * @param sibling[IN] the sibling node to split with. This node MUST be empty when this function is called.
 * @param midKey[OUT] the key in the middle after the split. This key should be inserted to the parent node.
 * @return 0 if successful. Return an error code if there is an error.
 */
RC BTNonLeafNode::insertAndSplit(int key, PageId pid, BTNonLeafNode& sibling, int& midKey)
{
    char *currentKeyCursor = buffer + 5 + 3 * sizeof(int);
    char *insertStart = buffer + 5 + 3 * sizeof(int);
    char *readStart = buffer + 5 + sizeof(int);
    char *currentPidCursor = currentKeyCursor + sizeof(int);
    char *middleKeyCursor;
    int sibKey, sibPid;                                             //1st siblint key & pid (used for initialization)
    PageId middlePid;
    int currentKey, count1, count2,tmp;
    
    count1 = getKeyCount();
    memcpy(&currentKey, currentKeyCursor, sizeof(int));
    
    if (pid < 0)
        return RC_INVALID_PID;
    
    else if (sibling.getKeyCount())
        return RC_INVALID_ATTRIBUTE;
    
    else{
        currentKeyCursor = buffer + 5 + sizeof(int);                                 //skip first pid
        currentPidCursor = currentKeyCursor + sizeof(int);
        for (int i = 0; i < count1; i++) {
            memcpy(&currentKey, currentKeyCursor, sizeof(int));
            if (key < currentKey) {
                memmove(currentKeyCursor + 2 * sizeof(int), currentKeyCursor, (count1 - i) * 2 * sizeof(int) );
                memmove(currentKeyCursor, &key, sizeof(int));
                memmove(currentPidCursor, &pid, sizeof(int));
                tmp++;
                break;
            }
            else{
                currentKeyCursor += 2 * sizeof(int);
                currentPidCursor += 2 * sizeof(int);
                if (i == count1 - 1) {
                    memmove(currentKeyCursor, &key, sizeof(int));
                    memmove(currentPidCursor, &pid, sizeof(int));
                    tmp++;
                }
            }
        }
    }
    count1++;
    middleKeyCursor = readStart + 2 * int(count1/2) * sizeof(int);
    memcpy(&midKey, middleKeyCursor, sizeof(int));
    memcpy(&middlePid, middleKeyCursor + sizeof(int), sizeof(int));
    //set sibling node
    count2 = count1 - int(count1/2) - 1;
    count1 = int(count1/2);
    memcpy(&sibKey, middleKeyCursor + 2 * sizeof(int), sizeof(int));
    memcpy(&sibPid, middleKeyCursor + 3 * sizeof(int), sizeof(int));
    sibling.initializeRoot(middlePid, sibKey, sibPid);
    sibling.setBuffer(insertStart + 2 * (count1 + 1 ) * sizeof(int) , 2 * (count2 - 1) * sizeof(int));
    //set original node
    memset(readStart + 2 * count1 * sizeof(int), '\0', 2 * sizeof(int) * (count2 + 1)  );
    memcpy(buffer + 1, &count1, sizeof(int));
    
    return 0;
}

/*
 * Given the searchKey, find the child-node pointer to follow and
 * output it in pid.
 * @param searchKey[IN] the searchKey that is being looked up.
 * @param pid[OUT] the pointer to the child node to follow.
 * @return 0 if successful. Return an error code if there is an error.
 */
RC BTNonLeafNode::locateChildPtr(int searchKey, PageId& pid)
{
    char * currentPidCursor = buffer + 5 ;
    char * currentKeyCursor = currentPidCursor + sizeof(int);
    int currentKey;

    for ( int i = 0; i < getKeyCount() ; i++ ) {
        

        memmove(&currentKey, currentKeyCursor, sizeof(int));
        
        if (currentKey > searchKey){
            memmove (&pid, currentPidCursor, sizeof(int));
            break;
        }
        else{
            if (i == getKeyCount() - 1){
            memmove (&pid, currentPidCursor + 2 * sizeof(int), sizeof(int));
            break;
            }
            currentPidCursor += 2 * sizeof(int);
            currentKeyCursor += 2 * sizeof(int);

        }
        
    }
    return 0;
    }

/*
 * Initialize the root node with (pid1, key, pid2).
 * @param pid1[IN] the first PageId to insert
 * @param key[IN] the key that should be inserted between the two PageIds
 * @param pid2[IN] the PageId to insert behind the key
 * @return 0 if successful. Return an error code if there is an error.
 */
RC BTNonLeafNode::initializeRoot(PageId pid1, int key, PageId pid2)
{
    char * cur = buffer + 5;
    int count = getKeyCount();
    memmove(cur, &pid1, sizeof(int));
    cur = cur + sizeof (int);
    memmove(cur, &key, sizeof(int));
    cur = cur + sizeof (int);
    memmove(cur, &pid2, sizeof(int));
    count++;
    memmove(buffer+1, &count, sizeof(int));
    return 0;
}

RC BTNonLeafNode::readentry(int eid, int& key, PageId& pid)
{
    memmove(&key, buffer+5+sizeof(PageId)+(sizeof(int)+sizeof(PageId))*(eid-1), sizeof(int));
    memmove(&pid, buffer+5+sizeof(PageId)+(sizeof(int)+sizeof(PageId))*(eid-1)+sizeof(int), sizeof(PageId));
    return 0;
}