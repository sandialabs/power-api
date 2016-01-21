/*
 * Copyright 2014-2016 Sandia Corporation. Under the terms of Contract
 * DE-AC04-94AL85000, there is a non-exclusive license for use of this work
 * by or on behalf of the U.S. Government. Export of this program may require
 * a license from the United States Government.
 *
 * This file is part of the Power API Prototype software package. For license
 * information, see the LICENSE file in the top level directory of the
 * distribution.
*/

#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include "router.h"
#include "server.h"
#include <sstream>


void* startRtrThread( void *);
void* startSrvrThread( void *);

struct Args {
	int argc;
	std::vector<char*> argv;
};

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void findArgs( std::string prefix, int argc, char** argv, Args& args );

int main( int argc, char* argv[] )
{
	int rc;
	struct Args rtrArgs;
	pthread_t rtrThread = 0;

	std::deque<pthread_t> srvrThreads;
	
	rtrArgs.argv.push_back( argv[0] );
	findArgs( "rtr", argc, argv, rtrArgs );
	pthread_mutex_init(&mutex,NULL);

	if ( rtrArgs.argv.size() > 2 ) {
		rc = pthread_mutex_lock(&mutex);	
		assert(0==rc);
		rc = pthread_create( &rtrThread, NULL, startRtrThread, &rtrArgs );  
		assert(0==rc);
	}

	int count = -1;
	while ( 1 ) {

		struct Args& srvrArgs = *new Args;
		srvrArgs.argv.push_back( argv[0] );

		std::string tmp("srvr");
		if ( count > -1 ) {
			std::ostringstream convert;
			convert << count;
			tmp += convert.str(); 
		}

		findArgs( tmp , argc, argv, srvrArgs );

		if ( srvrArgs.argv.size() > 2 ) {
			
			srvrThreads.resize( srvrThreads.size() + 1 );
			rc = pthread_mutex_lock(&mutex);	
			assert(0==rc);
			rc = pthread_create( &srvrThreads.back(), NULL, startSrvrThread, &srvrArgs );  
			assert(0==rc);
		} else {
			delete &srvrArgs;
			break;
		}

		++count;
	} 

	while ( srvrThreads.size() ) {
		//printf("wait for server thread to exit\n");
		rc = pthread_join( srvrThreads.back(), NULL );
		srvrThreads.pop_back();
		assert(0==rc);
	}

	if ( rtrThread ) {
		//printf("wait for router thread to exit\n");
		rc = pthread_join( rtrThread, NULL );
		assert(0==rc);
	}

	//printf("%s exit\n",argv[0]);

	return 0;
}

void* startRtrThread( void * _args)
{
	Args& args = *(Args*)_args;
	printf("start router\n");

	PWR_Router::Router rtr(args.argc, &args.argv[0] );

	int rc = pthread_mutex_unlock(&mutex);	
	assert(0==rc);

	return (void*) (unsigned long)rtr.work();
}

void* startSrvrThread( void * _args)
{
	Args& args = *(Args*)_args;

	printf("start server\n");

	PWR_Server::Server srvr(args.argc, &args.argv[0] );

	int rc = pthread_mutex_unlock(&mutex);	
	assert(0==rc);

	return (void*) (unsigned long)srvr.work();
}

void findArgs( std::string prefix, int argc, char* argv[], Args& args )
{
	prefix = "--" + prefix + ".";
	int len = prefix.size();

	//printf("prefix=%s\n",prefix.c_str());
	for ( int i = 0; i < argc; i++ ) {
		if ( 0 == strncmp(argv[i],prefix.c_str(),len) ) {

			//printf("'%s' -> ",argv[i]);
			memset( argv[i], '-', len );
			
			char *p = argv[i] + ( len - 2 );
			//printf("'%s'\n",p);
			args.argv.push_back( p );
		}
	}
	args.argc = args.argv.size();
	args.argv.push_back(NULL);
}
