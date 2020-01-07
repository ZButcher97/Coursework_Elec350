#include "sample_hardware.hpp"
#include "Networkbits.hpp"

//Network thread - responsible for listening for connectinos and responding with updated tempature values
void networktest()
{
    //This only runs when BOTH switches are pressed down

    return;
    
    lcd.cls();
    lcd.printf("Basic HTTP server example\n");
    
    //Configure an ethernet connection
    EthernetInterface eth;
    eth.set_network(IP, NETMASK, GATEWAY);
    eth.connect();
    lcd.printf("The target IP address is '%s'\n", eth.get_ip_address());
    
    //Now setup a web server
    TCPServer srv;           //TCP/IP Server
    TCPSocket clt_sock;      //Socket for communication
    SocketAddress clt_addr;  //Address of incoming connection
    
    /* Open the server on ethernet stack */
    srv.open(&eth);
    
    /* Bind the HTTP port (TCP 80) to the server */
    srv.bind(eth.get_ip_address(), 80);
    
    /* Can handle 5 simultaneous connections */
    srv.listen(5);
    
    //KEEP RESPONDING WHILE THE SWITCHES ARE PRESSED
    using namespace std;
    lcd.cls();
    lcd.printf("Open 10.0.0.1 in a browser\n");
    
    //Block and wait on an incoming connection
    srv.accept(&clt_sock, &clt_addr);
    printf("accept %s:%d\n", clt_addr.get_ip_address(), clt_addr.get_port());
    
    //Uses a C++ string to make it easier to concatinate
    string response;
    //This is a C string
    char ldr_str[64];
    
    //Read the temperature value - note that getParameters() is thread safe
    float temp = sensor.getTemperature();
    
    //Convert to a C String
    sprintf(ldr_str, "%5.3f", temp );
    printf("LDR: %5.3f\n\r", temp);
    
    //Build the C++ string response
    response = HTTP_MESSAGE_BODY1;
    response += ldr_str;
    response += HTTP_MESSAGE_BODY2;
    
    //Send static HTML response (as a C string)
    clt_sock.send(response.c_str(), response.size()+6);    

    printf("Release BOTH switches\n");
    lcd.printf("Release BOTH switches\n");
    while ((SW1 != 0) && (SW2 != 0));
    wait(0.5); //debounce
}