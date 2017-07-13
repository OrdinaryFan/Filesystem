#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#define BLOCKSIZE 1024
#define BLOCKNUM 1024
#define DISKSIZE (BLOCKNUM*BLOCKSIZE)
#define MAXBLOCKPERFILE 10 //file can occupy block num_max
#define FILENAMELENGTH 12
#define MAXFILENUMININODE 10

typedef struct Inode//i结点
{
	char filename[FILENAMELENGTH];//文件名称
	char type; //类型File = 0, Dir = 1
	int length;
	union{
		struct
		{
			int StoreBlock[MAXBLOCKPERFILE];
			int length;
		}txtfile;
		struct 
		{
			char name[FILENAMELENGTH];
			char type;
			int InodeNum;
		}NextInode[MAXFILENUMININODE];
	};
}Inode;

const int InodeMaxNum = BLOCKSIZE/sizeof(struct Inode);//共几个结点

struct DISK//文件卷的组织
{
	char BitMap[BLOCKSIZE];//一个char类型8位
	char BitMap_Inode[BLOCKSIZE];
	Inode InodeArea[2][BLOCKSIZE];
	char Data[BLOCKNUM-4][BLOCKSIZE];
};

struct UserOpenFile//打开文件结构
{
	char dir[FILENAMELENGTH*2];
	int dir_InodeNum;

	int block_num, block_offset;
	int file_InodeNum;
	char filename[FILENAMELENGTH];
};

int dispatchNewInode(int * InnodeNum,char type);
int dispatchNewBlock(int * StoreBlock);
void ls();
void mkdir(char* command, struct UserOpenFile * currentOpenFile);
void cd(char* command);
void create(char *command);
void read(char* command);
void write(char * command);
void freeBlock(int node);
int IsfreeNewInode();
int open(char * command, int *a, int *b);
int IsfreeNewBlock();
void freeInode(int node);
int rm(char*command);
void help();
void backup();
char* virDistAddress;
char currentdir[FILENAMELENGTH] = "/";
char cmd[10];
struct DISK * disk;
struct UserOpenFile openfilelist[10];
struct UserOpenFile * currentOpenFile;
char command[50];
char username[10] = "zjp";

void CreatNew()
{
	memset(virDistAddress, 0, DISKSIZE);
	disk->BitMap[0] = 15;
	memset(disk->BitMap_Inode, 0, sizeof(disk->BitMap_Inode));
	disk->BitMap_Inode[0] = 1;
	disk->InodeArea[0][0].type = 1;
	strcpy(disk->InodeArea[0][0].filename, "/");
	disk->InodeArea[0][0].length = 2;
	strcpy(disk->InodeArea[0][0].NextInode[0].name,".");
	disk->InodeArea[0][0].NextInode[0].InodeNum = 0;
	disk->InodeArea[0][0].NextInode[0].type = 1;
	strcpy(disk->InodeArea[0][0].NextInode[1].name,"..");
	disk->InodeArea[0][0].NextInode[1].InodeNum = 0;
	disk->InodeArea[0][0].NextInode[1].type = 1;
}

int initialSystem()
{
	virDistAddress = (char*)malloc(DISKSIZE);
	disk = (struct DISK*)virDistAddress;
	

	FILE* fp = fopen("FileSystem","r+");
	if(fp != NULL)
	{
		fread(virDistAddress, sizeof(char), DISKSIZE, fp);
		fclose(fp);
	}
	else
	{
		CreatNew();
	}
	memset(openfilelist, 0, sizeof(openfilelist));
	strcpy(openfilelist[0].dir, "/");
	openfilelist[0].dir_InodeNum = 0;
	currentOpenFile = &(openfilelist[0]);
	strcpy(currentdir, "/");
	return 0;
}

int main(int argc, char const *argv[])
{
	initialSystem();

	int i ;
	while(1)
	{
		printf("\033[1;32m");//高亮绿色
		printf("@%s", username);
		printf("\033[0m");//恢复
		printf(":");
		printf("\033[1;34m");//高亮蓝色
		printf("%s", currentOpenFile->dir);
		printf("\033[0m");
		printf("$ ");
		scanf("%s",cmd);
		if(!strcmp(cmd,"ls"))
		{
			ls();
		}
		else if(!strcmp(cmd,"mkdir"))
		{
			scanf("%s", command);
			mkdir(command, currentOpenFile);
		}
		else if(!strcmp(cmd,"cd"))
		{
			scanf("%s",command);
			cd(command);
		}
		else if(!strcmp(cmd,"create"))
		{
			scanf("%s",command);
			create(command);
		}
		else if(!strcmp(cmd,"read"))
		{
			scanf("%s",command);
			read(command);
		} 
		else if(!strcmp(cmd,"write"))
		{
			scanf("%s",command);
			write(command);
		} 
		else if(!strcmp(cmd,"rm"))
		{
			scanf("%s",command);
			rm(command);
		}
		else if(!strcmp(cmd,"exit"))
		{
			backup();
			break;
		}
		else if(!strcmp(cmd,"help"))
		{
			help();
		}
		else printf("command not find!\n");
	}
	return 0;
}


void ls()
{
	int x = currentOpenFile->dir_InodeNum / InodeMaxNum;
	int y = currentOpenFile->dir_InodeNum % InodeMaxNum;
	int len = disk->InodeArea[x][y].length;
	int i;
	printf("\033[1;34m");
	printf(".  ..  ");
	printf("\033[0m");
	for(i = 2;i < len; i++)
	{
		if(disk->InodeArea[x][y].NextInode[i].type == 1){
			printf("\033[1;32m");//高亮绿色
			printf("%s  ", disk->InodeArea[x][y].NextInode[i].name);
			printf("\033[0m");
		}
		else
			printf("%s  ", disk->InodeArea[x][y].NextInode[i].name);
		if((i+1)%5 == 0 && (i+1)!=len)
			printf("\n");
	}
	printf("\n");
}
  

void mkdir(char* command,struct UserOpenFile *currentOpenFile)
{
	int x = currentOpenFile->dir_InodeNum / InodeMaxNum;
	int y = currentOpenFile->dir_InodeNum % InodeMaxNum;
	int len = disk->InodeArea[x][y].length;
	strcpy(disk->InodeArea[x][y].NextInode[len].name, command);
	disk->InodeArea[x][y].NextInode[len].type = 1;
	dispatchNewInode(&(disk->InodeArea[x][y].NextInode[len].InodeNum),(char)1);
	disk->InodeArea[x][y].length ++;
	
	len = disk->InodeArea[x][y].NextInode[len].InodeNum;
	x = len / InodeMaxNum;
	y = len % InodeMaxNum;
	disk->InodeArea[x][y].type = 1;
	strcpy(disk->InodeArea[x][y].filename,command);
	disk->InodeArea[x][y].length = 2;
	strcpy(disk->InodeArea[x][y].NextInode[0].name, ".");
	disk->InodeArea[x][y].NextInode[0].InodeNum = len;
	disk->InodeArea[x][y].NextInode[0].type = 1;
	strcpy(disk->InodeArea[x][y].NextInode[1].name, "..");
	disk->InodeArea[x][y].NextInode[1].InodeNum = currentOpenFile->dir_InodeNum;
	disk->InodeArea[x][y].NextInode[1].type = 1;
}

int IsfreeNewInode()
{
	int i,j;
	int temp;
	for(i = 0;i < MAXBLOCKPERFILE*2;i++)
	{
		if(disk->BitMap_Inode[i] == 0)
		{
			return 1;
		}
	}
	return 0;
}

int dispatchNewInode(int * InodeNum,char type)
{
	int i,j;
	int temp;
	// if(type == 1)
	for(i = 0;i < MAXBLOCKPERFILE*2;i++)
	{
		if(disk->BitMap_Inode[i] == 0)
		{
			disk->BitMap_Inode[i] = 1;
			*InodeNum = i;
			return 1;
		}
	}
	return 0;
}

int IsfreeNewBlock()
{
	int i,j,temp;
	for(i = 0;i < BLOCKSIZE;i++)
	{
		temp = disk->BitMap[i];
		for(j = 0;j < 8;j++)
		{
			if(temp & 1 == 1)
				temp = temp >> 1;
			else 
			{
				return 1;
			} 
		}
	}
	return 0;
}

int dispatchNewBlock(int * StoreBlock)
{
	int i,j,temp;
	for(i = 0;i < BLOCKSIZE;i++)
	{
		temp = disk->BitMap[i];
		for(j = 0;j < 8;j++)
		{
			if(temp & 1 == 1)
				temp = temp >> 1;
			else
			{
				temp = temp | 1;
				*StoreBlock = 8*i+j;
				for(;j > 0;j--)
					temp = temp << 1;
				disk->BitMap[i] |=  temp;
				return 1;
			} 
		}
	}
	return 0;
}

void cd(char* command)
{
	int x,y,i,len,k; 
	x = currentOpenFile->dir_InodeNum / InodeMaxNum;
	y = currentOpenFile->dir_InodeNum % InodeMaxNum;
	if(!strcmp(command,"."))
		return ;
	if(!strcmp(command,".."))
	{
		if(!strcmp(currentOpenFile->dir, "/"))//屏蔽根目录
			return ;
		len = strlen(currentOpenFile->dir);
		i = 0;
		for(k = 0;k < len;k++)
		{
			if(currentOpenFile->dir[k] == '/')
			{
				i = k;
			}
		}
		if(i == 0)	strcpy(currentOpenFile->dir, "/");
		else
		{
			currentOpenFile->dir[i] ='\0';
		}
		currentOpenFile->dir_InodeNum = disk->InodeArea[x][y].NextInode[1].InodeNum;
		return ;
	}
	
	for(i = 0;i < disk->InodeArea[x][y].length;i++)
	{
		if(!strcmp(disk->InodeArea[x][y].NextInode[i].name, command))
		{
			if(disk->InodeArea[x][y].NextInode[i].type == 0)//屏蔽cd打开非文件夹
			{
				printf("\033[0;32;31m");
				printf("Error : not dir!\n");
				printf("\033[0m");
				return ;
			}
			currentOpenFile->dir_InodeNum = disk->InodeArea[x][y].NextInode[i].InodeNum;
			currentOpenFile->file_InodeNum = -1;
			if(strcmp(currentOpenFile->dir,"/"))//疑问
				strcat(currentOpenFile->dir,"/");
			strcat(currentOpenFile->dir, command);
			return ;
		}
	}
	printf("\033[0;32;31m");
	printf("Error : can't find \n");
	printf("\033[0m");
}

void create(char * command)
{
	int StoreBlockNum, InodeNum, x, y;
	if(IsfreeNewInode() == 0)
	{
		printf("\033[0;32;31m");
		printf("Error : no free InodeArea\n");
		printf("\033[0m");
		return;
	}
	if(dispatchNewBlock(&(StoreBlockNum)) == 0)
	{
		printf("\033[0;32;31m");
		printf("Error : no free space!\n");
		printf("\033[0m");
		return ;
	}
	dispatchNewInode(&(InodeNum),(char)0);
	x = currentOpenFile->dir_InodeNum / InodeMaxNum;
	y = currentOpenFile->dir_InodeNum % InodeMaxNum;
	int len = disk->InodeArea[x][y].length ;
	//add file in dir
	disk->InodeArea[x][y].length ++;  
	disk->InodeArea[x][y].NextInode[len].InodeNum = InodeNum;
	strcpy(disk->InodeArea[x][y].NextInode[len].name, command);
	disk->InodeArea[x][y].NextInode[len].type = 0;
	//add file in Inode
	x = InodeNum / InodeMaxNum;
	y = InodeNum % InodeMaxNum;
	strcpy(disk->InodeArea[x][y].filename, command);
	disk->InodeArea[x][y].type = 0;
	disk->InodeArea[x][y].txtfile.length = 1;
	disk->InodeArea[x][y].txtfile.StoreBlock[0] = StoreBlockNum;
	disk->Data[StoreBlockNum][0] = '\0';
}

int open(char * command,int * a,int * b)
{
	int i, x, y, len;
	x = currentOpenFile->dir_InodeNum / InodeMaxNum;
	y = currentOpenFile->dir_InodeNum % InodeMaxNum;
	len = disk->InodeArea[x][y].length ;
	for(i = 0;i < len;i++)
	{
		if(!strcmp(disk->InodeArea[x][y].NextInode[i].name, command))//找到对应文件
		{
			if(disk->InodeArea[x][y].NextInode[i].type == 1)
			{
				printf("\033[0;32;31m");
				printf("Error : not a file\n");
				printf("\033[0m");
				return -1;
			}
			currentOpenFile->file_InodeNum = disk->InodeArea[x][y].NextInode[i].InodeNum;
			strcpy(currentOpenFile->filename, command);
			x = currentOpenFile->file_InodeNum / InodeMaxNum;
			y = currentOpenFile->file_InodeNum % InodeMaxNum;	
			currentOpenFile->block_num = disk->InodeArea[x][y].txtfile.StoreBlock[0];
			currentOpenFile->block_offset = 0;
			*a = x;
			*b = y;
			return 0;
		}
	}
	printf("\033[0;32;31m");
	printf("Error : can't find!\n");
	printf("\033[0m");
	return -2;
}

void freeBlock(int node)
{
	int i,j,temp;
	j = node % 8;
	i = node / 8;
	temp = 1;
	for(; j > 0; j--)
		temp = temp << 1;
		temp = ~temp;
	disk->BitMap[i] &= temp;
	return ;
}

void read(char * command)
{
	int x, y, i, j, pos = 0;
	char buffer[3000], temp_buf[BLOCKSIZE];
	if(open(command, &x, &y) == 0)
	{
		for(i = 0; i < disk->InodeArea[x][y].txtfile.length; i++)
		{
			memcpy(temp_buf, disk->Data[disk->InodeArea[x][y].txtfile.StoreBlock[i]], BLOCKSIZE);
			for(j = 0;j < BLOCKSIZE; j++)
			{
				buffer[pos] = temp_buf[j];
				pos++;
			}
		}
		printf("%s\n", buffer);	
	}
	return ;
}

void update()
{
	FILE * fp = fopen("MyFileSystem", "w");
	fwrite(virDistAddress,sizeof(char),DISKSIZE,fp);
	fclose(fp);
	//free(virDistAddress);
	return;
}

void write(char* command)
{
	int x,y,len,block_offset = 0,pos = 0,i,No = 0;
	int StoreBlockNum;
	char buffer[3000], temp_buf[BLOCKSIZE];
	if(open(command,&x,&y) == 0)
	{
		getchar();
		for(i = 0;i < disk->InodeArea[x][y].txtfile.length;i++)
		{
			freeBlock(disk->InodeArea[x][y].txtfile.StoreBlock[i]);
		}
		currentOpenFile->block_num = disk->InodeArea[x][y].txtfile.StoreBlock[0];
		while(scanf("%[^/32]", buffer) != EOF)
		{
			len = strlen(buffer);
			buffer[len] = '\0';
			pos = 0;
			while(len > pos)
			{	
				for(i = block_offset;i < BLOCKSIZE  && pos < len;i++)
				{
					temp_buf[i] = buffer[pos];
					pos++;
				}
				if(i == BLOCKSIZE || pos == len) //full or over
				{
					dispatchNewBlock(&StoreBlockNum);
					memcpy(disk->Data[StoreBlockNum],temp_buf,BLOCKSIZE);
					disk->InodeArea[x][y].txtfile.StoreBlock[No++] = StoreBlockNum;
				}
				block_offset = (len - pos) % BLOCKSIZE;			
			}
		}
		if(block_offset != 0)
		{
			dispatchNewBlock(&StoreBlockNum);
			memcpy(disk->Data[StoreBlockNum], temp_buf, block_offset);
			disk->InodeArea[x][y].txtfile.StoreBlock[No++] = StoreBlockNum;
		}
		disk->InodeArea[x][y].txtfile.length = No;
	}
	//update();
	return ;
}

void freeInode(int node)
{
	disk->BitMap_Inode[node] = 0;
}

int rm(char*command)
{
	if(!strcmp(command,"..") || !strcmp(command,"."))
	{
		printf("\033[0;32;31m");
		printf("Error: can't rm special file!\n");
		printf("\033[0m");
		return 0;
	}
	int i,x,y,len,InodeNum,j;
	x = currentOpenFile->dir_InodeNum / InodeMaxNum;
	y = currentOpenFile->dir_InodeNum % InodeMaxNum;
	len = disk->InodeArea[x][y].length ;
	for(i = 0;i < len;i++)
	{
		if(!strcmp(disk->InodeArea[x][y].NextInode[i].name,command))
		{
			if(disk->InodeArea[x][y].NextInode[i].type == 1)
			{
				int temp = disk->InodeArea[x][y].NextInode[i].InodeNum;
				int x = temp / InodeMaxNum;
				int y = temp % InodeMaxNum;
				if(disk->InodeArea[x][y].length != 2)
				{
					printf("\033[0;32;31m");
					printf("Error : not a empty dir,have %d file(s)!\n",disk->InodeArea[x][y].length-2);
					printf("\033[0m");
					return -1;
				}
			}
			freeInode(disk->InodeArea[x][y].NextInode[i].InodeNum);
			for(j = i;j < len -1;j++)
			{
				disk->InodeArea[x][y].NextInode[j] = disk->InodeArea[x][y].NextInode[j+1];
			}
			disk->InodeArea[x][y].length --;
			return 0;
		}
	}
	printf("\033[0;32;31m");
	printf("Error : can't find!\n");
	printf("\033[0m");
	return -2;
}

void backup()
{
	FILE * fp = fopen("FileSystem", "w");
	fwrite(virDistAddress, sizeof(char), DISKSIZE, fp);
	fclose(fp);
	free(virDistAddress);
	return ;
}

void help(){
	printf(  "   *********************************************************\n");
	printf(  "   * mkdir xx: to create a folder named xx                 *\n");
	printf(  "   * ls: View the current folder file                      *\n");
	printf(  "   * create xx: to create a file named xx                  *\n");
	printf(  "   * read xx: to read the file named xx                    *\n");
	printf(  "   * write xx: to write the file named xx(Ctrl+D to end)   *\n");
	printf(  "   * rm xx: to delete the file named xx                    *\n");
	printf(  "   * exit: to exit and save the FileSystem                 *\n");
	printf(	 "   *********************************************************\n");
	return ;
}

