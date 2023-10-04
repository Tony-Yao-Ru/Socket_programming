#include<stdio.h>
#include<stdlib.h>
#include<arpa/inet.h>
#include<string.h>

#define IP_LEN 15
#define MAX_MASK_LEN 32
#define COMPLEMENT(num) (num = num^0xFFFFFFFF)
#define SET_BIT(n, pos) (n = n | 1<<pos)
#define UNSET_BIT(n, pos) (n = n & ( (1<<pos) ^ 0xFFFFFFFF ))

static unsigned int generate_bits_regarding_mask(char mask_value) {
	
	unsigned int mask_arr = 0xFFFFFFFF;
	
	char bit0s_len = MAX_MASK_LEN - mask_value;
	//printf("Initially: mask_arr = %010x\n", mask_arr);

	for(int i=0; i<bit0s_len; i++){
		UNSET_BIT(mask_arr, i);
	}

	//printf("Finally: mask_arr = %010x\n", mask_arr);

	return mask_arr;
}

void get_network_id(char *ip_addr, char mask, char *network_id) {

	unsigned int mask_integer = generate_bits_regarding_mask(mask);
	unsigned int ip_addr_integer = 0;
	unsigned int network_id_integer = 0;
	
	inet_pton(AF_INET, ip_addr, &ip_addr_integer);
	ip_addr_integer = htonl(ip_addr_integer);
	network_id_integer = ip_addr_integer & mask_integer;
	network_id_integer = htonl(network_id_integer);
	inet_ntop(AF_INET, &network_id_integer, network_id, IP_LEN+1);
}


void get_broadcast_addr(char *ip_addr, char mask, char *brdcst_ip_addr) {
	
	unsigned int ip_addr_integer = 0;
	unsigned int mask_integer = 0;
	unsigned int brdcst = 0;

	/*Convert ip address from A.B.C.D format to unsigned integer*/
	inet_pton(AF_INET, ip_addr, &ip_addr_integer);
	//printf("In func get_broad: ip_addr_integer = %zu\n", ip_addr_integer);
	ip_addr_integer = htonl(ip_addr_integer);
	//printf("In func get_broad: ip_addr_intger = %010x\n", ip_addr_integer);

	mask_integer = generate_bits_regarding_mask(mask);
	COMPLEMENT(mask_integer);
	brdcst = ip_addr_integer | mask_integer;
	brdcst = htonl(brdcst);
	
	inet_ntop(AF_INET, &brdcst, brdcst_ip_addr, IP_LEN+1);
	brdcst_ip_addr[IP_LEN] = '\0';
}



int main(int argc, char **argv){
	char testcase[] = "192.168.2.10";
	char mask = 24;
	char ip_addr[IP_LEN+1], network_id[IP_LEN+1],  brdcst_ip_addr[IP_LEN+1];
	
	memset(ip_addr, 0, IP_LEN+1);
	memcpy(ip_addr, testcase, strlen(testcase));
	ip_addr[strlen(ip_addr)] = '\0';	
	memset(brdcst_ip_addr, 0, IP_LEN+1);
	memset(network_id, 0, IP_LEN+1);
	
	printf("This ip is %s\n", testcase);
	printf("The mask is %d\n", mask);
	printf("\n");

	/*Testing get_network_id()*/
        {
                printf("Testing Q1 starts: \n");
                get_network_id(ip_addr, mask, network_id);
                printf("network ID = %s/%u\n", network_id, mask);
                printf("Testing Q1 Done.\n");
        }


	/*Testing get_broadcast_address()*/
	{	
		printf("Testing Q2 starts: \n");
		get_broadcast_addr(ip_addr, mask, brdcst_ip_addr);
		printf("broadcast IP address = %s\n", brdcst_ip_addr);
		printf("Testing Q2 Done.\n");
	}
	
	return 0;
}






