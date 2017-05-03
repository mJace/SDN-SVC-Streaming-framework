#include "streamer.h"
#include "traceline.h"
#include <pthread.h>


int sock1, sock2, sock3;   //3 socket for 3 udp port.
FILE *outvideofile = NULL;
static int writeLock = 0;
static int last = 0;

int decodepacket(struct traceline *newtl, struct ourpacket *pkt, FILE *outvideofile)
{

	newtl->length = ntohs(pkt->total_size) - HEADER_SIZE;
	newtl->lid = pkt->lid;
	newtl->tid = pkt->tid;
	newtl->qid = pkt->qid;
	newtl->startpos = ntohl(pkt->naluid);

	switch (pkt->flags & STREAMER_MASK_NALU_TYPE) {
	case STREAMER_NALU_TYPE_STREAMHEADER:
		newtl->packettype = TRACELINE_PKT_STREAMHEADER;
		break;
	case STREAMER_NALU_TYPE_PARAMETERSET:
		newtl->packettype = TRACELINE_PKT_PARAMETERSET;
		break;
	case STREAMER_NALU_TYPE_SLICEDATA:
		newtl->packettype = TRACELINE_PKT_SLICEDATA;
		break;
	default:
		newtl->packettype = TRACELINE_PKT_UNDEFINED;
	}

	if (!((pkt->flags & STREAMER_MASK_DISCARDABLE) ^ STREAMER_NALU_DISCARDABLE)) {
		newtl->discardable = TRACELINE_YES;
	}
	else {
		newtl->discardable = TRACELINE_NO;
	}
	if (!((pkt->flags & STREAMER_MASK_TRUNCATABLE) ^ STREAMER_NALU_TRUNCATABLE)) {
		newtl->truncatable = TRACELINE_YES;
	}
	else {
		newtl->truncatable = TRACELINE_NO;
	}
	newtl->frameno = ntohs(pkt->frame_number);
	if (fwrite(pkt->payload, 1, newtl->length, outvideofile) < newtl->length)
	{
		fprintf(stderr, "receiver.c: %s\n", strerror(errno));
		return -1;
	}
	fflush(outvideofile);
	return 0;
}

void quitreceiver(int sig) {
	fprintf(stderr, "Video time expired. Quitting...\n");
	//traceline_free(&tl);
	//close(sock);
	exit(0);
}

void quitonsig(int sig) {
	fprintf(stderr, "\nQuitting...\n");
	fflush(stderr);
	//traceline_free(&tl);
	//close(sock);
	exit(10);
}

unsigned long timeval2ulong(struct timeval *tv) {
	/* time in millis */
	unsigned long ret;
	ret = 1e3 * tv->tv_sec;
	ret += 1e-3 * tv->tv_usec;
	return ret;
}

void lock(void) {  //A simple lock for file write in multi-thread.
	while (writeLock == 1) {
	}
	writeLock = 1;
}

void unlock(void) {
	writeLock = 0;
}

void *receiveThread(void *i) {    //the input i is the socket.
	int ret;
	int sock = (int)i;
	struct timeval tv;
	struct traceline *tl;
	struct traceline *newtl;
	struct ourpacket *recpacket;
	struct ourpacket *secondpacket;
	char recvbuffer[MAX_PAYLOAD];


	memset(&recvbuffer, 0, MAX_PAYLOAD);
	tl = (struct traceline *) malloc(sizeof(struct traceline));
	tl->next = NULL;
	tl->prev = NULL;
	newtl = tl;

	while (1) {
		ret = recvfrom(sock, (void *)recvbuffer, MAX_PAYLOAD, 0, NULL, NULL);
		if (ret <= 0) {
			if (ret < 0) {
				fprintf(stderr, "recvfrom: %s\n", strerror(errno));
			}
			else {
				fprintf(stderr, "recvfrom: The peer has performed an orderly shutdown. Quitting.\n");
			}
			traceline_free(&tl);
			close(sock);
			exit(5);
		}
		gettimeofday(&tv, NULL);
		recpacket = (struct ourpacket *) recvbuffer;
		
		lock();  //hang here if locked.

		if (decodepacket(newtl, recpacket, outvideofile) != 0) {
			traceline_free(&tl);
			close(sock);
			exit(6);
		}
		newtl->timestamp = timeval2ulong(&tv);
		traceline_print_one(stdout, newtl);

		if (!((recpacket->flags & STREAMER_MASK_TWONALUS) ^ STREAMER_NALU_TWONALUS)) {
			newtl->next = (struct traceline *) malloc(sizeof(struct traceline));
			newtl->next->next = NULL;
			newtl->next->prev = newtl;
			newtl = newtl->next;
			secondpacket = (struct ourpacket *)(&recvbuffer[ntohs(recpacket->total_size)]);
			if (decodepacket(newtl, secondpacket, outvideofile) != 0)
			{
				traceline_free(&tl);
				close(sock);
				exit(7);
			}
			newtl->timestamp = timeval2ulong(&tv);
			traceline_print_one(stdout, newtl);
			if (!((secondpacket->flags & STREAMER_MASK_LAST) ^ STREAMER_NOT_LAST_PACKET)) {
				newtl->next = (struct traceline *) malloc(sizeof(struct traceline));
				newtl->next->next = NULL;
				newtl->next->prev = newtl;
				newtl = newtl->next;
			}
			else {
				fprintf(stderr, "Stream finished (2)\n");  //get the final packet.
				newtl->next = NULL;
				last = 1;
			}
		}
		else {
			if (!((recpacket->flags & STREAMER_MASK_LAST) ^ STREAMER_NOT_LAST_PACKET)) {
				newtl->next = (struct traceline *) malloc(sizeof(struct traceline));
				newtl->next->next = NULL;
				newtl->next->prev = newtl;
				newtl = newtl->next;
			}
			else {
				fprintf(stderr, "Stream finished\n");
				newtl->next = NULL;
				last = 1;
			}
		}
		
		unlock();  //unlock when file write finished.
		
	}
	traceline_free(&tl);
}

int main(int argc, char **argv) {
	int b1, b2, b3;
	struct sockaddr_in sin1, sin2, sin3;
	//char recvbuffer[MAX_PAYLOAD];
	//struct ourpacket *recpacket;
	//struct ourpacket *secondPacket;
	//struct traceline *newtl;
	char *filename;
	int videoduration;
	int alarmset = 0;
	pthread_t t1, t2, t3;

	if (argc < 4) {
		fprintf(stderr, "Usage: %s <listening port> <output H.264 file> <video duration in milliseconds>\n", argv[0]);
		exit(1);
	}

	signal(SIGALRM, quitreceiver);
	signal(SIGTERM, quitonsig);
	signal(SIGINT, quitonsig);

	filename = argv[2];
	videoduration = atoi(argv[3]);

	fprintf(stderr, "Starting...\n");
	sock1 = socket(AF_INET, SOCK_DGRAM, 0);
	sock2 = socket(AF_INET, SOCK_DGRAM, 0);
	sock3 = socket(AF_INET, SOCK_DGRAM, 0);

	if (sock1 < 0 || sock2 < 0 || sock3 < 0) {
		fprintf(stderr, "receiver.c socket: %s\n", strerror(errno));
		exit(1);
	}
	memset(&sin1, 0, sizeof(sin1));
	memset(&sin2, 0, sizeof(sin2));
	memset(&sin3, 0, sizeof(sin3));

	sin1.sin_family = AF_INET;
	sin1.sin_addr.s_addr = htonl(INADDR_ANY);
	sin1.sin_port = htons(atoi(argv[1]));      //udp port for 1st layer

	sin2.sin_family = AF_INET;
	sin2.sin_addr.s_addr = htonl(INADDR_ANY);
	sin2.sin_port = htons(atoi(argv[1]) + 1);  //udp port for 2nd layer

	sin3.sin_family = AF_INET;
	sin3.sin_addr.s_addr = htonl(INADDR_ANY);
	sin3.sin_port = htons(atoi(argv[1]) + 2);  //udp port for 3rd layer

	fprintf(stderr, "Binding...\n");
	b1 = bind(sock1, (struct sockaddr *) &sin1, sizeof(sin1));
	b2 = bind(sock2, (struct sockaddr *) &sin2, sizeof(sin2));
	b3 = bind(sock3, (struct sockaddr *) &sin3, sizeof(sin3));
	if (b1 < 0 || b2 < 0 || b3 < 0) {
		fprintf(stderr, "receiver.c bind: %s\n", strerror(errno));
		close(sock1);
		close(sock2);
		close(sock3);
		exit(3);
	}

	outvideofile = fopen(filename, "w"); // open videofile for write data
	if (outvideofile == NULL)
	{
		fprintf(stderr, "receiver.c fopen: %s\n", strerror(errno));
		close(sock1);
		close(sock2);
		close(sock3);
		exit(4);
	}

	if (alarmset == 0)
	{
		//alarm(videoduration);
		struct itimerval itv; //linux c 中的timer結構
		itv.it_value.tv_sec = (long)(videoduration / 1000.0); //secounds
		itv.it_value.tv_usec = (videoduration % 1000) * 1000;  //micro seconds
		setitimer(ITIMER_REAL, &itv, NULL);
		alarmset = 1;
	}


	pthread_create(&t1, NULL, &receiveThread, (void *)sock1);
	pthread_create(&t2, NULL, &receiveThread, (void *)sock2);
	pthread_create(&t3, NULL, &receiveThread, (void *)sock3);

	while (!last) {
	}

	close(sock1);
	close(sock2);
	close(sock3);
	fclose(outvideofile);
	
	return 0;
}
