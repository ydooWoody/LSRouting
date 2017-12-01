Project 3: Simulating a Link State Routing Protocol
Created by:
Chris Marques
Ryan Cox
Nikolay Radaev


Creation:
        We started off giving each other goals to create the threading, create the socket/send/receive logic, and create the parser. These starting points streamlined the process. We did the same strategy when it came to moving forward with TCP, UDP, and table creations. Each of us worked through the given task and worked together to get everything working as a whole. Giving each other documentation and starter methods really helped in the combinations throughout the project. 
 
Implementation:
Manager.cpp-
1. File is parsed and important data is added to classes in the header (Link, srcDest,  etc)
2. Methods for creating sockets, sending and receiving were built to streamline operations
3. Router threads are created for each router using the router constructor. This constructor runs the router side version of runManager.
4. This method then calls the main runManager method for controlling the routers.
5. This method runs near everything flow wise. Everything is given a state to achieve and a system to get to that state. Ie, for the manager to send packets, it sends all of them and waits for an ACK. Each ACK decrements a counter that eventually will break from this loop and move on to the next step. All loops in this method are similar to this.
6. The first loop relies on getRouterNeighbors which determines what links to send a given router based on the router number.
7. From here, everything is send/receive using the aforementioned TCP methods for doing the heavy lifting. 
8. Select acception is done using acceptAny, which looks at connections and selects one from what was being listened to.
9. The function getTime uses a library and gets the current time as a string.


Router.cpp-
1. When a router is created with the constructor, run() is called which creates a UDP port and socket.
2. The router uses createTCPSocket to connect to manager and send UDP information
3. The router now creates an always listening thread to monitor UDP messages and use them as needed.
4. The router enters createTCPSocket and begins waiting for messages from the manager until quit
5. Each else-if in this method handles a different task similarly to runManager. Depending on the message it receives, it sends an ACK and can either send packets back to the manager or send packets to it’s neighbors.
6. The set of methods for TCP sockets in manager is similar to the methods in router for UDP, streamlining the operations.
7. buildForward, sendPacket, linkRequest, and the broadcast methods all handle different situations for sending packets. The names are self-explanatory. 
8. Dupe, and the Graph related methods are helpers. Dupe checks for duplicates in the forwarding table before they are added. The Graph methods and class are for basic dijkstras. These find the shortest path given a forwarding table and send the shortest Links to the algorithm method. This algorithm method handles the forwarding table -> routing table need in createTCPSocket. This algorithm method makes a call for each router based on their forwarding table to recursionMF. This method recurses through the list of links given a destination and the forwarding table and returns a hop towards the routing table.


Status and Problems:
Everything at this point is working 100%. The manager creates routers and sends them links. When a router receives the links they request the links from their neighbors and broadcasts this info to all other neighbors. This builds a forwarding table and later a routing table to determine next hop for any given packet. The manager sends packets to a router and the router uses this table to send packets to the next hop. Routers are all receiving the packets that are sent to them via any others. The process for the example input.txt works in around 10 seconds.


The only problem we have is the occasional badSocket error or segfault on startup. These happen very seldom but do occur if a thread manages to beat the manager to launch. Again, this only happens maybe once out of 10 runs (sometimes the error persists afterwards for 1-5 attempts given the error the first). A run will work correctly if the console states it is broadcasting.


Log notes:
* Lines beginning with “-----” are headers for a sector and don't include times, this is because the call right after is near the exact same time frame and the structure is more organized without times on them.
* In the log for manager, thread creating beats the output at the beginning, the output is incorrect sequentially but the actual timing is correct.
* In manager.log, 1 word messages are typically ACKs with specific names so that we understood where each was originating.
* Similarly to manager, in router.log the output is beat towards packet sending because packets are sent quicker than the log can keep up. The output is non-sequential, but in actuality is correct.
* The packets can be tracked by messages starting with ‘~’ and the lines that say where a packet was sent. 
* A given packet will be shown as received with “**** X *****” in the destination router’s log.
