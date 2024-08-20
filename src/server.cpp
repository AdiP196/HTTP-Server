#include <iostream>
#include <cstdlib>
#include <string>
#include <cstring> //Conține funcții ca strlen, strcpy, memcpy
#include <unistd.h> //Contine funcții ca read, write, close, ope, exit
#include <sys/types.h>
#include <sys/socket.h> //include definitii si declaratii necesare pentru socket-uri
#include <arpa/inet.h>  //definitii pentru conversia adreselor de retea
#include <netdb.h>
#include <sstream>
#include <thread>
#include <vector>
#include <filesystem>
#include <fstream>
//  Un socket este un canal generalizat de comunicare între procese, reprezentat în Linux/UNIX print-un descriptor de fișiere
//  Simplist, un file descriptor este un număr întreg ce reprezintă un identificator în tabela de fișiere a unui program.
//  De exemplu, dacă deschidem un fișier, identificatorul ar putea fi numărul 4.
//  Fiecare proces are propria sa tabelă de file descriptor
//  Un program are inițial 3 file descriptors: 0 - stdin, 1 - stdout si 2 - stderr

std::string directory;

void handle_client(int client_fd){
  std::string client_message(1024, '\0');
  ssize_t brecvd = recv(client_fd, (void *)&client_message[0], client_message.max_size(), 0);
  if (brecvd < 0)
  {
    std::cerr << "error receiving message from client\n";
    close(client_fd);
    return ;
  }
  std::cerr << "Client Message (length: " << client_message.size() << ")" << std::endl;
  std::clog << client_message << std::endl;
  std::string response;

  std::istringstream request_stream(client_message);
  std::string method, path, http_version;
  request_stream >> method >> path >> http_version;

  // User Agent
  std::string user_agent;
  std::string line;

  while (std::getline(request_stream, line) && line != "\r\n")
  {
    if (line.find("User-Agent:") == 0)
    {
      user_agent = line.substr(12);
      break;
    }
  }

  if (path == "/")
  {
    response = "HTTP/1.1 200 OK\r\n\r\n";
  }
  else if (path.rfind("/echo/") == 0)
  {
    std::string string = path.substr(6);
    response = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length:" + std::to_string(string.size()) + "\r\n\r\n" + string;
  }
  else if (path == "/user-agent")
  {
    response = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length:" + std::to_string(user_agent.size() - 1) + "\r\n\r\n" + user_agent;
  }
  else
  {
    response = "HTTP/1.1 404 Not Found\r\n\r\n";
  }

  // Handle files
  std::string filename = path.substr(7);
  std::string filepath = directory + "/" + filename;
  std::ifstream inputFile(filepath, std::ios::binary); // Open file in binary mode

  inputFile.seekg(0, std::ios::end);
  std::streamsize size = inputFile.tellg();
  inputFile.seekg(0, std::ios::beg);
  std::vector<char> file_content(size);
  inputFile.read(file_content.data(), size);
  std::string content(file_content.begin(), file_content.end());
  
  if (std::filesystem::exists(filepath))
  {

    response = "HTTP/1.1 200 OK\r\nContent-Type: application/octet-stream\r\nContent-Length:" + std::to_string(size) + "\r\n\r\n" + content;
  }else{
    response = "HTTP/1.1 404 Not Found\r\n\r\n";
  }

  ssize_t bsent = send(client_fd, response.c_str(), response.size(), 0);
  if (bsent < 0)
  {
    std::cerr << "error sending response to client\n";
    close(client_fd);
    return ;
  }

    close(client_fd);
}
int main(int argc, char **argv)
{
  directory = argv[2];//?????????

  int server_fd = socket(AF_INET, SOCK_STREAM, 0);
  // server_fd = file descriptor pentru socket; In caz de eroare intoarce -1 si se seteaza variabila errno
  // AF_INET: It specifies the IPv4 protocol family
  // SOCK_STREAM: It defines that the TCP type socket
  // 0 selectează protocolul implicit pentru SOCK_STREAM(TCP)
  if (server_fd < 0){
    std::cerr << "Failed to create server socket\n";
    return 1;
  }
  // Since the tester restarts your program quite often, setting REUSE_PORT
  // ensures that we don't run into 'Address already in use' errors
  int reuse = 1;
  // We set the socket option to reuse the address to 1
  if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEPORT, &reuse, sizeof(reuse)) < 0)
  {
    // SOL_SOCKET -> We pass the "level" SOL_SOCKET bc we want to reuse the addr
    // SO_REUSEPORT allow reuse of local addresses(take an int val, it is a bool option)
    // reuse = A pointer to the buffer in which the value for the requested option is specified
    // The size, in bytes, of the buffer pointed to by the optval parameter
    std::cerr << "setsockopt failed\n";
    close(server_fd);
    return 1;
  }
  struct sockaddr_in server_addr;
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = INADDR_ANY;
  server_addr.sin_port = htons(4221);
  // Host order: Big-endian(MSB la cea mai mica addr) and Little-endian(LSB la cea mai mica addr)
  // Network byte order use Big-endian
  // Htons make the conversion to big-endian - host to netwok short

  // Bind asociaza o adresa pentru sockeul deschis
  if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) != 0)
  {
    std::cerr << "Failed to bind to port 4221\n";
    close(server_fd);
    return 1;
  }
  int connection_backlog = 5;
  if (listen(server_fd, connection_backlog) != 0)
  {
    std::cerr << "listen failed\n";
    close(server_fd);
    return 1;
  }
  std::vector<std::thread> threads;
  while(true){
    struct sockaddr_in client_addr;
    int client_addr_len = sizeof(client_addr);
    std::cout << "Waiting for a client to connect...\n";
    int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, (socklen_t *)&client_addr_len);
    if (client_fd < 0)
    {
      std::cerr << "error handling client connection\n";
      continue;  
    }
  threads.emplace_back(handle_client,client_fd);
  }
  for (auto &th : threads)
  {
    if (th.joinable())
    {
      th.join();
    }
  }
  close(server_fd);
  return 0;
}