# RenkuOS Wi-Fi neighbor-discovery diagnostic

On the N100 physical test image, the MediaTek interface associated with the
access point and obtained an IPv4 address. The user reported three successful
pings from the laptop to the router. The SSH daemon was running and listening
on IPv4 and IPv6 port 22; its log showed successful public-key authentication
when the workstation could reach it.

A workstation-side test removed only that workstation's temporary neighbor
entry and sent four pings. All failed while a packet capture observed repeated
ARP requests for the laptop without replies. After the already observed laptop
MAC was installed in the workstation neighbor table, five pings and SSH passed.
A second host on the wired side also reached the laptop when given the known
MAC. This confirms that the observed failure precedes SSH and that unicast can
work when neighbor resolution is bypassed. It does not establish whether ARP
broadcast delivery, driver receive filtering, the network stack, or access-point
configuration is responsible.

A non-promiscuous packet-capture attempt on RenkuOS reported zero captured
packets and kernel drops; it is not sufficient to identify where ARP requests
were lost. Earlier, a short captured test worked with promiscuous mode enabled,
but its effect is not proven. Further controlled tests should compare ARP
requests at the access point and the RenkuOS receive path, with promiscuous mode
and wireless isolation controlled, and should preserve the private network
configuration. No Wi-Fi driver fix has been installed or claimed.

As a temporary workstation-only workaround, the laptop's observed MAC was
placed in the Linux neighbor table. This mapping is not a persistent network
configuration and will need re-establishment after the interface resets or its
address changes. The SSH configuration continues to validate the laptop's host
key.
