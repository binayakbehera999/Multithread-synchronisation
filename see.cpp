#include <iostream>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <vector>
#include <queue>
#include <fstream>
#include <sstream>
#include <string>
#include <pthread.h>

using namespace std;
int n; //no services in the system
int m; // no of threads/workers in each service
int execDone=0;
pthread_mutex_t outputMtx;
struct worker
{
    int threadId;
    int serviceId;
    int priority;
    int resources;
    bool isBusy;
    pthread_cond_t wakeup;
    pthread_mutex_t self;

};

struct request
{
    int transactionType; // equates to service id
    int resourceNeed;
    int waitCount=0;
};

struct service
{
    int serviceId;
    pthread_mutex_t self;
    vector<worker>workers;
    queue<request>reqQue;
};

vector<service>services;
queue<request>commonQue;
void commonPoolAllocater()
{
    //common que dont need mutex cuz only this thread acceses it
    while(!commonQue.empty())
    {
        request r= commonQue.front();
        commonQue.pop();
        int sid= r.transactionType; //service type of the queue that i need to allocate to
        pthread_mutex_lock(&services[sid].self);
        services[sid].reqQue.push(r);
        pthread_mutex_unlock(&services[sid].self);

    }
}
struct passArg
{
    int workerId;
    int serviceId;
};

void* workerThread(void* arg)
{
    passArg* p= (passArg*) arg;
    int workerId= p->workerId;
    int serviceId= p->serviceId;

    //each worker goes to sleep as soon as it is made and waits for signal to wakeup
    //when i want to allocate a thread i will make it wakeup and take value from que of
    //the service it belongs to
    chrono::seconds delay(1);
    while(1)
    {
        pthread_mutex_lock(&services[serviceId].workers[workerId].self);
        while(services[serviceId].reqQue.empty())
        {
            pthread_cond_wait(&services[serviceId].workers[workerId].wakeup,&services[serviceId].workers[workerId].self);
            if(execDone) pthread_exit(NULL);
        }
        //if it comes here then we have woken up this worker and we have something for it in the que
        services[serviceId].workers[workerId].isBusy=true;    
          
        request r= services[serviceId].reqQue.front();
        services[serviceId].reqQue.pop();
        pthread_mutex_unlock(&services[serviceId].self);//other workers can be allocated
        //worker fullfills this request r

        this_thread::sleep_for(delay);
        cout<<"worker "<<workerId<<" of service "<<serviceId<<" has served request\n";
        services[serviceId].workers[workerId].isBusy=false; 
        pthread_mutex_unlock(&services[serviceId].workers[workerId].self);  
        
    }
}

void serviceScheduler(int serviceId)
{
    while(1)
    {
        pthread_mutex_lock(&services[serviceId].self);
        queue<request> currQ= services[serviceId].reqQue;
        if(currQ.empty())
        {
            //unlock 
            pthread_mutex_unlock(&services[serviceId].self);
            return;
        }
        request r= currQ.front();
        // we now find the id of highest priority worker that can satisfy r
        int id=-1;
        int priority=-1; //higher no more prioirty
        for(auto x : services[serviceId].workers)
        {
            if(x.isBusy)continue;
            if(x.resources>= r.resourceNeed && x.priority>priority) {id= x.threadId; priority=x.priority;}
        }
        if(id==-1)
        {
            if(r.waitCount>10)
            {
                cout<<"this request is being rejected which needed "<< r.resourceNeed<<" resources\n";
                services[serviceId].reqQue.pop();
            }
            else 
            {
                 //i will say that this request cannot be allocated right now
                cout<<"curr request cannot be allocated right now\n";
                services[serviceId].reqQue.front().waitCount++;
                services[serviceId].reqQue.pop();
                services[serviceId].reqQue.push(r);
                //push it to back of que
            }
            pthread_mutex_unlock(&services[serviceId].self);
            continue;
           

        }
        //now we wakeup the worker with id, and it will execute the process in front of que
        pthread_cond_signal(&services[serviceId].workers[id].wakeup);
    }
}
void* serviceThread(void* arg)
{
    int serviceId= *((int*)arg);
    //each service thread creates  m worker threads
    pthread_t workerThreads[m];
    for(int i=0;i<m;i++)
    {
        passArg p;
        p.serviceId=serviceId;
        p.workerId=i;
        pthread_create(&workerThreads[i],NULL,workerThread,(void*)&p);
    }
    //we have all workers of this service ready now, 
    //now we run the scheduler for this thread and pick worker threads to execute
    serviceScheduler(serviceId);
   /* for(int i=0;i<m;i++)
    {
        pthread_join(workerThreads[i],NULL);
    }
    */
    pthread_mutex_lock(&outputMtx);
    cout<<"All threads of service id: "<<serviceId<<" have come back\n";
    pthread_mutex_unlock(&outputMtx);
   
}
bool allEmpty()
{
    if(!commonQue.empty()) return false;
    int tes=0;
    for(int i=0;i<n;i++)
    {
        pthread_mutex_lock(&services[i].self);
        if(!services[i].reqQue.empty()) tes=1;
        pthread_mutex_unlock(&services[i].self);
    }
    if(tes)return false;
    return true;
}
int main()
{
    pthread_mutex_init(&outputMtx,NULL);
    FILE* file1 = freopen("input.txt", "r", stdin);
    FILE* file2 = freopen("output.txt", "w", stdout);
    cin>>n;
    cin>>m;
    services= vector<service>(n);

    for(int i=0;i<n;i++)
    {
        services[i].serviceId=i;
        pthread_mutex_init(&services[i].self,NULL);

        services[i].workers= vector<worker>(m);

        
        for(int j=0;j<m;j++)
        {
            services[i].workers[j].serviceId=i;
            services[i].workers[j].threadId=j;
            services[i].workers[j].isBusy=false;
            pthread_mutex_init(&services[i].workers[j].self,NULL);
            pthread_cond_init(&services[i].workers[j].wakeup,NULL);
            cin>>services[i].workers[j].priority;
            cin>>services[i].workers[j].resources;

        }
    }
    ifstream inputFile("requests.txt"); 
    if (inputFile.is_open()) 
    {
    string line;

    while (getline(inputFile, line)) 
    { 
      int num1, num2;
      stringstream ss(line); 
      request s1;
      ss >> s1.transactionType >> s1.resourceNeed; 
      commonQue.push(s1);
    }

    inputFile.close(); 
    }
    else cout<<"error opening file\n";

    //now run the request allocater from common que to individual
    commonPoolAllocater();

    //now lets create all the service threads
    pthread_t serviceThreads[n];
    for(int i=0;i<n;i++)
    {
        int a = i;
        pthread_create(&serviceThreads[i],NULL,serviceThread,(void*)&a);
    }
    //now we make functionality that checks all ques including common que
    //if all are empty, time to terminate all threads and end programme
    //also need to display stats before ending
    while(!allEmpty());
    execDone=1;
    //wakeup all worker threads if any sleeping and have them exit
    /*for(int i=0;i<n;i++)
    {
        for(int j=0;j<m;j++)
        {
            pthread_cond_signal(&services[i].workers[j].wakeup);
        }
    }*/
    /*for(int i=0;i<n;i++)
    {
        pthread_join(serviceThreads[i],NULL);
    }*/
    pthread_mutex_lock(&outputMtx);
    cout<<"All service threads are back\n";
    pthread_mutex_unlock(&outputMtx);
    freopen("/dev/tty", "r", stdin);
    freopen("/dev/tty", "w", stdout);

}