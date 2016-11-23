SDN SVC streaming framework.
===================

This SDN SVC streaming framework is based on Dr. Chih-Heng Ke's myEvalSVC-Mininet [1]
I modified the streamer and receiver to let SVC streaming send packets on 3 UDP ports (total 3 layers of SVC stream). By doing so, SDN controller can setup flow tables for each layer of SVC stream (based on different ports) and separate their routing path for better transmission quality.

  [1]: http://csie.nqu.edu.tw/smallko/sdn/myEvalSVC-Mininet.htm

