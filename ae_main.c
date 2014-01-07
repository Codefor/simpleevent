#include "ae.h"

#include <stdio.h>
#include <time.h>

int serverCron(struct aeEventLoop *eventLoop, long long id, void *clientData){
	fprintf(stderr,"serverCron:%ld\n",time(NULL));
	/*the retval is delay time before next fired*/
	return 100;
}

int main(){
    aeEventLoop *el = aeCreateEventLoop(1024);

    if(aeCreateTimeEvent(el, 1000, serverCron, NULL, NULL) == AE_ERR) {
	}

	/* start loop */
	aeMain(el);
	aeDeleteEventLoop(el);
	return 0;
}
