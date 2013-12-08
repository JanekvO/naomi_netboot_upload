/*
**	Made (or rather, converted) by JustJanek
**
**	Compiled using GCC on Ubuntu/Linux. 
**	Might work with MingwGCC or CygwinGCC on Windows but hasn't been tested thus far.
**	
**	Might not be the prettiest but it works. The original python netboot code (the one I used to 
**  make this) is more detailed and has more functionality but if you only used it to upload 
**	games to the net dimm then this will suffice.
**
**	"What's the point?" you might ask. First off, it takes away the time necessary finding out how
**	python works, what version of python you need and what libraries you need to scour the internet over
**	to find. When I finally got my net dimm I was excited but it still took a couple of weeks finding out
**	how things worked and what to install. If you got this as binary you're good to go.
**	Secondly, if you're like me and want to expand on this netboot upload tool (e.g. make some automatic
**	upload system using a web interface) but want to use C rather than python you now can using this
**  maybe you will have to add some externally available function in this file to allow other .c files
**	to upload a game.
*/

#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <memory.h>
#include <stdlib.h>
#include <unistd.h>

#include "crc32.h"
#include "datatypes.h"

#define port 		10703
#define MAXDATASIZE 256
/* Max. size of buffer used to send over the game */
#define BUFFER_SIZE 0x8000

/* socket file descriptor */
static int socket_fd;

/* Local declarations */
static INT_32 read_socket(char *recv_buf);
static void set_security_keycode(UINT_64 data);
static INT_32 set_mode_host(char *recv_buf);
static void set_information_dimm(UINT_32 crc, UINT_32 length);
static void upload_dimm(UINT_32 addr, char *buff, INT_32 mark, UINT_32 buff_size);
static void upload_file_dimm(char* gameFile);
static void restart_host();
static void set_time_limit(UINT_32 data);
static void print_usage();


int main(int argc, char **argv)
{
	struct sockaddr_in naomi_address;
	char *recv_buf;
	INT_32 recv_len;
	INT_32 i;
	
	if(argc != 3)
	{
		print_usage(argv[0]);
		return 1;
	} 
	if(access(argv[2], F_OK | R_OK))
	{
		printf("Error: file not found or not accessible\n");
		return 1;
	}

	memset(&naomi_address, 0x00, sizeof(struct sockaddr_in));
	if(inet_aton(argv[1], (struct in_addr *) &naomi_address.sin_addr.s_addr) == 0) 
	{
        print_usage(argv[0]);
        return 1;
    }
	naomi_address.sin_family = AF_INET;
	naomi_address.sin_port = htons(port);

	if ((socket_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) 
	{
        printf("Error: socket initialization\n");
		return 1;
    }
	
	if (connect(socket_fd, (struct sockaddr *) &naomi_address, sizeof(struct sockaddr_in)) != 0) 
	{
        close(socket_fd);
		printf("Error: connecting\n");
        return 1;
    }
	
	recv_buf = malloc(MAXDATASIZE);
	
	recv_len = set_mode_host(recv_buf);
	if(recv_len == -1)
	{
		free(recv_buf);
		return 1;
	}
	
	printf("Received: '");
	for(i = 0; i < recv_len; i++)
	{
		printf("0x%02x ", recv_buf[i]);
	}
	printf("'\n");
	
	free(recv_buf);
	
	set_security_keycode(0);
	
	upload_file_dimm(argv[2]);
	
	restart_host();
	
	printf("Entering infinite loop\n");
	while(1)
	{
		set_time_limit(10*60*1000); 						/* Don't hurt me if this still doesn't work. D= Just converting all from python*/
		sleep(5);
	}
	
}

/*
**	Description: Prints the usage explanation
**
**	Parameters: [*binary_name]
**					Name of the executable
**
*/
void print_usage(char *binary_name)
{
	printf("Usage: %s <ip> <game file>\n", binary_name);
	printf("E.g.: %s 192.168.0.120 TetrisKiwamemichi_v6.bin\n", binary_name);
}

/*
**	Description: the recv function to receive packet(s) from the net dimm
**
**	Parameters: [*buffer]
**					pointer to allocated memory of size MAXDATASIZE
**  
**	Return: Size of received data in bytes
**
*/
INT_32 read_socket(char *recv_buffer)
{
	INT_32 buf_len = recv(socket_fd, recv_buffer, MAXDATASIZE-1, 0);
	if (buf_len == -1) 
	{
		printf("Error: read_socket\n");
		return -1;
	}
	
	return buf_len;
}

/*
**	Description: Sets the keycode data, in most cases make this 0 for the magic key (whatever that means)
**
**	Parameters: [data]
**					Keycode data
**
*/
void set_security_keycode(UINT_64 data)
{	
	struct __attribute__((__packed__)) packet_struct		/* My system at time of programming this is 64-bit aligned so we're packing the struct */
	{
		UINT_32 opcode;
		UINT_64 data;
	} packet;
	
	packet.opcode = 0x7F000008;
	packet.data = data;
	
	if (send(socket_fd, &packet, sizeof(struct packet_struct), 0) == -1)
	{
		printf("Error: sending in set_security_keycode\n");
	}
}

/*
**	Description: 
**
**	Parameters: [*recv_buffer]
**					Pointer to allocated memory of MAXDATASIZE
**  
**	Return: Size of received data in bytes
**
*/
INT_32 set_mode_host(char *recv_buffer)
{ 
	struct packet_struct
	{
		UINT_32 opcode;
		UINT_32 data;
	} packet;
	
	packet.opcode = 0x07000004;
	packet.data = 1;
	
	if (send(socket_fd, &packet, sizeof(struct packet_struct), 0) == -1)
	{
		printf("Error: sending in set_mode_host\n");
		return -1;;
	}
	
	return read_socket(recv_buffer);
}

/*
**	Description: Send information about the crc and length to the net dimm
**
**	Parameters: [crc]
**					crc32 of the game data
**				[length]
**					size of the game data
**
*/
void set_information_dimm(UINT_32 crc, UINT_32 length)
{
	struct packet_struct
	{
		UINT_32 opcode;
		UINT_32 crc;
		UINT_32 len;
		UINT_32 packet_data;
	} packet;
	
	packet.opcode = 0x1900000C;
	packet.crc = crc;
	packet.len = length;
	packet.packet_data = 0;
	
	if (send(socket_fd, &packet, sizeof(struct packet_struct), 0) == -1)
	{
		printf("Error: sending in set_information_dimm\n");
	}
}

/*
**	Description: Uploads actual chunk of game data
**
**	Parameters: [addr]
**					Offset to indicate how many are send thus far
**				[*buff]
**					Pointer to chunk of data to be send
**				[mark]
**					No f****** idea
**				[buff_size]
**					Size of the data where *buff points at
**
*/
void upload_dimm(UINT_32 addr, char *buff, INT_32 mark, UINT_32 buff_size)
{
	struct __attribute__((__packed__)) packet_struct 		/* Would've packed it even if it was a 32 bit system */
	{
		UINT_32 opcode_with_info;
		UINT_32 packet_data_1;
		UINT_32 packet_data_2;
		UINT_16 packet_data_3;
		char 	game_data[buff_size];						/* Necessary to put this data after the other packet data */
	} packet;
	
	packet.opcode_with_info = 0x04800000 | (buff_size + 10) | (mark << 16);
	packet.packet_data_1 = 0;
	packet.packet_data_2 = addr;
	packet.packet_data_3 = 0;
	memcpy(packet.game_data, buff, buff_size);
	
	if (send(socket_fd, &packet, sizeof(struct packet_struct), 0) == -1)
	{
		printf("Error: sending in upload_dimm\n");
	}
}

/*
**	Description: Upload file to net dimm and send the appropriate information
**
**	Parameters: [gameFile]
**					Character array of gameFile
**
*/
void upload_file_dimm(char* gameFile)
{	
	UINT_32 address = 0;
	UINT_32 crc = 0;
	UINT_32 char_read;
	char buff[BUFFER_SIZE];
	
	FILE * game_file = fopen(gameFile,"rb");
	
	printf("Uploading...\n");
	while(1)
	{
		printf("\rAddress at: %08x", address);
		char_read = fread(buff, sizeof(char), BUFFER_SIZE, game_file);	/* Take a chunk of size BUFFER_SIZE data from the file gameFile */
		if(!char_read)
			break;
		upload_dimm(address, buff, 0, char_read);						/* Upload the chunk of data in buff to the net dimm */
		crc = crc32(crc, buff, char_read);								/* Keep track of 32 bit CRC */
		address += char_read;											/* Keep track of the characters read from file */
	}
	fclose(game_file);
	printf("\n");
	
	crc = ~crc;
	upload_dimm(address, "12345678", 1, 9); 	/* Not quite sure what's going on here. I know it's sending 
													'1','2','3','4','5','6','7','8','\0' to the net dimm. Why? no idea. */
	set_information_dimm(crc, address);			/* Send over the 32 bit CRC*/
}

/*
**	Description: Restarts the Naomi/Triforce/Chihiro
**
*/
void restart_host()
{
	UINT_32 packet = 0x0A000000;
	
	if (send(socket_fd, &packet, sizeof(UINT_32), 0) == -1)
	{
		printf("Error: sending in restart_host\n");
	}
}

/*
**	Description: To set the time limit. Not sure what time limit we're talking about.
**
**	Parameters: [data]
**					The time to set in 32 bits unsigned int.
**
*/
void set_time_limit(UINT_32 data)
{
	struct packet_struct
	{
		UINT_32 opcode;
		UINT_32 data;
	} packet;
	
	packet.opcode = 0x17000004;
	packet.data = data;
	
	if (send(socket_fd, &packet, sizeof(struct packet_struct), 0) == -1)
	{
		printf("Error: sending in set_time_limit\n");
	}
}
