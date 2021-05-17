#ifndef WIN32
   #include <arpa/inet.h>
   #include <netdb.h>
#else
   #include <winsock2.h>
   #include <Windows.h>
   #include <ws2tcpip.h>
#endif
#include <fstream>
#include <iostream>
#include <cstdlib>
#include <cstring>
#include <udt.h>
#include <string.h>
#include <WinSock.h>

using namespace std;


//关闭控制台
#pragma comment(linker, "/subsystem:\"windows\" /entry:\"wmainCRTStartup\"" )

DWORD WINAPI runsoft(LPVOID soft)
{
	char softpos[1024] ={0};
	strcpy(softpos,(char *)soft);
	//cout <<"softpos:"<< softpos << endl;
	char filepos[1024] = "cmd.exe /c ";
	strcat(filepos,softpos);
	//cout << "filepos" << filepos << endl;
	WinExec(filepos, SW_HIDE);
	return 0;
}


int wmain(int argc, char* argv[])
{
	char ip_path[256] = { 0 }; //获取服务端ip
	FILE *fp_ip = NULL;
	fp_ip = fopen("C\:\\recvapp\\tool\\ip.txt", "r");
	//fp_ip = fopen("C:\\ip\\ip.txt", "r");
	if (fp_ip == NULL)
		return -1;
	int ret;
	while (1)//读取ip
	{
		memset(ip_path, 0, 256);
		if (ret = fread(ip_path, sizeof(char), 256, fp_ip) != 0);//一个一个读取	
		break;
	}
	fseek(fp_ip, 0, SEEK_SET);
	fclose(fp_ip);//关闭文件
	cout << "IP :" << ip_path << endl;

	UDT::startup(); //udt初始化
	cout << "StartUp is ok" << endl;
	//地址信息分配
	struct addrinfo hints, *peer;
	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_flags = AI_PASSIVE;
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;

	//UDT套接字创建
	UDTSOCKET fhandle = UDT::socket(hints.ai_family, hints.ai_socktype, hints.ai_protocol);
	// 根据服务器地址信息提示分配实际可用的地址信息
	//例如：if (0 != getaddrinfo("10.33.21.11", "8899", &hints, &peer))//服务器ip，设置固定链接端口

	if (0 != getaddrinfo(ip_path, "8899", &hints, &peer))//服务器ip，设置固定链接端口
	{
		cout << "incorrect server/peer address. " << "10.33.21.11" << ":" << "8899" << endl;
		return 0;
	}
	cout << "get server ip is success!" << endl;

	if (UDT::ERROR == UDT::connect(fhandle, peer->ai_addr, peer->ai_addrlen))//尝试建立连接
	{
		cout << "server is not start : " << UDT::getlasterror().getErrorMessage() << endl;//连接超时，休眠6s结束本次循环
		return 0;
	}
	freeaddrinfo(peer);

	//开始软件接收
	int len = 0;
	char file[1024] = { 0 };

	if (UDT::ERROR == UDT::recv(fhandle, (char*)&len, sizeof(int), 0))
	{
		cout << "file length: " << UDT::getlasterror().getErrorMessage() << endl;
		return 0;
	}
	if (UDT::ERROR == UDT::recv(fhandle, file, len, 0))
	{
		cout << "filename: " << UDT::getlasterror().getErrorMessage() << endl;
		return 0;
	}

	file[len] = '\0';
	cout << "recv filename: " << file << endl;

	//char oldfile[1024] = { 0 };//根据文件名，取决是否接收。
	/*if (strcmp(oldfile, file) == 0) {
		cout << "同一个文件，已经接收完成！休眠一分钟" << endl;
		return 0;
	}*/

	//strcpy(oldfile, file);
	//cout << "oldfile" << oldfile << endl;

	// get size information
	int64_t size;

	if (UDT::ERROR == UDT::recv(fhandle, (char*)&size, sizeof(int64_t), 0))
	{
		cout << "recv size:" << UDT::getlasterror().getErrorMessage() << endl;
		return 0;
	}

	if (size < 0)
	{
		cout << "file size:" << UDT::getlasterror().getErrorMessage() << endl;
		return 0;
	}

	cout << "filesize : " << size / 1024 / 1024 << "MB" << endl;

	char local_file[1024] = "C:\\recvapp\\";
	strcat(local_file, file);
	cout << "file save path : " << local_file << endl;

	//是否重接收确认（文件名和大小相同拒绝接收）
	fstream local(local_file, ios::in | ios::binary);

	local.seekg(0, ios::end);
	int64_t localsize = local.tellg();
	local.seekg(0, ios::beg);

	cout << "localsize = " << localsize <<endl;

	if(localsize == size)
	{
		cout << "软件已经存在！" << endl;
		local.close();
		return 0;
	}
	local.close();


	// receive the file
	fstream ofs(local_file, ios::out | ios::binary | ios::trunc);

	int64_t recvsize;
	int64_t offset = 0;

	if (UDT::ERROR == (recvsize = UDT::recvfile(fhandle, ofs, offset, size)))
	{
		cout << "File transfer error ：" << UDT::getlasterror().getErrorMessage() << endl;
		ofs.close();
		return 0;//发送端断开，重新连接服务器
	}
	
	ofs.close();
	cout << file << "recv file success!" << endl;
	//开线程运行接收到的程序
	HANDLE hThread = CreateThread(NULL, 0, runsoft, (void *)local_file, 0, NULL);

	WaitForSingleObject(hThread, INFINITE); // 等待，直到线程被激发

	//UDT::close(fhandle);//关闭此次连接
	//UDT::cleanup();
	//cout << "The recvfile.exe run ends" << endl;
	Sleep(1000);
	return 0;
}

