This is a HTTP web server project:

myhttpd:                                                       
                                                              
HTTP program that allows clients to communicate with server   
using sockets. (Concurrency allowed)                           
                                                              
                                                              
Username: wpt                                                
Password: 12345                                              
                                                
To use it:                                                    
1) In one window type:                                       
                                                            
      myhttpd [-f|-t|-p] [<port>]                            
                                                          
   -f: process mode                                        
   -t: thread mode                                          
   -p: pool of threads                         .             
   1024 < port < 65536.                                      
                                                           
2) Else, you can type:                                      
                                                           
      myhttpd [<port>]                                     
                                                           
   Where 1024 < port < 65536.                               
   And it opens in default iterative mode.                 
                                                             
3) Else, you can type:                                        
                                                           
      myhttpd                                               
                                                          
   Where default port is set to be \"9104\".               
   And it opens in default iterative mode.                 
                                                              
                                                            
                                                            
In another window:                                         
1) You can type:                                             
                                                              
   telnet <host> <port>                                      
                                                              
2) Else, you can use Chrome/Firefox, and type:               
                                                               
   data.cs.purdue.edu: <port>                                
                                                             
Where <host> is the name of the machine where myhttpd         
is running. And <port> is the port number you used             
when you run myhttpd.                                          
