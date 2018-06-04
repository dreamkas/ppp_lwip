//
// Created by d.eroshenkov on 04.05.2018.
//

/* C runtime includes */
#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <string.h>
#include <conio.h>

/* lwIP core includes */
#include "lwip/opt.h"

#include "lwip/sys.h"
#include "lwip/timeouts.h"
#include "lwip/debug.h"
#include "lwip/stats.h"
#include "lwip/init.h"
#include "lwip/tcpip.h"
#include "lwip/netif.h"
#include "lwip/api.h"

#include "lwip/tcp.h"
#include "lwip/udp.h"
#include "lwip/dns.h"
#include "lwip/dhcp.h"
#include "lwip/autoip.h"

/* lwIP netif includes */
#include "lwip/etharp.h"
#include "netif/ethernet.h"


#if LWIP_USING_NAT

#include "lwip_nat/ipv4_nat.h"

#endif

/* applications includes */
#include "lwip/apps/lwiperf.h"
#include "lwip/apps/netbiosns.h"
#include "lwip/apps/sntp.h"
#include "lwip/apps/httpd.h"
#include "lwip/apps/mdns.h"
//#include "apps/httpserver/httpserver-netconn.h"
//#include "apps/netio/netio.h"
//#include "apps/ping/ping.h"
//#include "apps/rtp/rtp.h"
//#include "apps/chargen/chargen.h"
//#include "apps/shell/shell.h"
//#include "apps/tcpecho/tcpecho.h"
//#include "apps/udpecho/udpecho.h"
//#include "apps/tcpecho_raw/tcpecho_raw.h"
//#include "apps/socket_examples/socket_examples.h"

#if NO_SYS
/* ... then we need information about the timer intervals: */
#include "lwip/ip4_frag.h"
#include "lwip/igmp.h"
#endif /* NO_SYS */

#include "netif/ppp/ppp_opts.h"

#if PPP_SUPPORT
/* PPP includes */
#include "lwip/sio.h"
#include "netif/ppp/pppapi.h"
#include "netif/ppp/pppos.h"
#include "netif/ppp/pppoe.h"

#if !NO_SYS && !LWIP_PPP_API
#error With NO_SYS==0, LWIP_PPP_API==1 is required.
#endif
#endif /* PPP_SUPPORT */

/* include the port-dependent configuration */
#include "lwipcfg_msvc.h"

/** Define this to 1 to enable a PCAP interface as default interface. */
#ifndef USE_PCAPIF
#define USE_PCAPIF 1
#endif

/** Define this to 1 to enable a PPP interface. */
#ifndef USE_PPP
#define USE_PPP 1
#endif

///** Define this to 1 or 2 to support 1 or 2 SLIP interfaces. */
//#ifndef USE_SLIPIF
//#define USE_SLIPIF 0
//#endif

/** Use an ethernet adapter? Default to enabled if PCAPIF or PPPoE are used. */
#ifndef USE_ETHERNET
#define USE_ETHERNET  (USE_PCAPIF || PPPOE_SUPPORT)
#endif

/** Use an ethernet adapter for TCP/IP? By default only if PCAPIF is used. */
#ifndef USE_ETHERNET_TCPIP
#define USE_ETHERNET_TCPIP  (USE_PCAPIF)
#endif

#if USE_ETHERNET

#include "pcapif.h"

#endif /* USE_ETHERNET */
#if USE_SLIPIF
#include <netif/slipif.h>
#endif /* USE_SLIPIF */

#ifndef USE_DHCP
#define USE_DHCP    LWIP_DHCP
#endif
#ifndef USE_AUTOIP
#define USE_AUTOIP  LWIP_AUTOIP
#endif

/* globales variables for netifs */
#if USE_ETHERNET
/* THE ethernet interface */
struct netif netif;
#if LWIP_DHCP
/* dhcp struct for the ethernet netif */
struct dhcp netif_dhcp;
#endif /* LWIP_DHCP */
#if LWIP_AUTOIP
/* autoip struct for the ethernet netif */
struct autoip netif_autoip;
#endif /* LWIP_AUTOIP */
#endif /* USE_ETHERNET */
#if USE_PPP
/* THE PPP PCB */
ppp_pcb *ppp;
/* THE PPP interface */
struct netif ppp_netif;
/* THE PPP descriptor */
u8_t sio_idx = 0;
sio_fd_t ppp_sio;
#endif /* USE_PPP */
#if USE_SLIPIF
struct netif slipif1;
#if USE_SLIPIF > 1
struct netif slipif2;
#endif /* USE_SLIPIF > 1 */
#endif /* USE_SLIPIF */

#if LWIP_USING_NAT
ip_nat_entry_t nat_entry;
#endif


#if USE_PPP

#include <stdbool.h>

extern volatile bool isPPPConnected;
volatile bool callClosePpp;
volatile bool isLwipInitialized;
volatile bool isIpInterfaceInitialized;

static void pppLinkStatusCallback(ppp_pcb *pcb, int errCode, void *ctx)
{
    struct netif *pppif = ppp_netif(pcb);
    LWIP_UNUSED_ARG(ctx);

    switch (errCode)
    {
        case PPPERR_NONE:
        {             /* No error. */
            printf("pppLinkStatusCallback: PPPERR_NONE\n");
#if LWIP_IPV4
            printf("   our_ipaddr  = %s\n", ip4addr_ntoa(netif_ip4_addr(pppif)));
            printf("   his_ipaddr  = %s\n", ip4addr_ntoa(netif_ip4_gw(pppif)));
            printf("   netmask     = %s\n", ip4addr_ntoa(netif_ip4_netmask(pppif)));
#endif /* LWIP_IPV4 */
#if LWIP_DNS
            printf("   dns1        = %s\n", ipaddr_ntoa(dns_getserver(0)));
            printf("   dns2        = %s\n", ipaddr_ntoa(dns_getserver(1)));
#endif /* LWIP_DNS */
#if PPP_IPV6_SUPPORT
            printf("   our6_ipaddr = %s\n", ip6addr_ntoa(netif_ip6_addr(pppif, 0)));
#endif /* PPP_IPV6_SUPPORT */

            isPPPConnected = true;

            break;
        }
        case PPPERR_PARAM:
        {           /* Invalid parameter. */
            printf("pppLinkStatusCallback: PPPERR_PARAM\n");
            break;
        }
        case PPPERR_OPEN:
        {            /* Unable to open PPP session. */
            printf("pppLinkStatusCallback: PPPERR_OPEN\n");
            break;
        }
        case PPPERR_DEVICE:
        {          /* Invalid I/O device for PPP. */
            printf("pppLinkStatusCallback: PPPERR_DEVICE\n");
            break;
        }
        case PPPERR_ALLOC:
        {           /* Unable to allocate resources. */
            printf("pppLinkStatusCallback: PPPERR_ALLOC\n");
            break;
        }
        case PPPERR_USER:
        {            /* User interrupt. */
            printf("pppLinkStatusCallback: PPPERR_USER\n");
            break;
        }
        case PPPERR_CONNECT:
        {         /* Connection lost. */
            printf("pppLinkStatusCallback: PPPERR_CONNECT\n");
            break;
        }
        case PPPERR_AUTHFAIL:
        {        /* Failed authentication challenge. */
            printf("pppLinkStatusCallback: PPPERR_AUTHFAIL\n");
            break;
        }
        case PPPERR_PROTOCOL:
        {        /* Failed to meet protocol. */
            printf("pppLinkStatusCallback: PPPERR_PROTOCOL\n");
            break;
        }
        case PPPERR_PEERDEAD:
        {        /* Connection timeout */
            printf("pppLinkStatusCallback: PPPERR_PEERDEAD\n");
            isPPPConnected = false;
            callClosePpp = true;
            break;
        }
        case PPPERR_IDLETIMEOUT:
        {     /* Idle Timeout */
            printf("pppLinkStatusCallback: PPPERR_IDLETIMEOUT\n");
            break;
        }
        case PPPERR_CONNECTTIME:
        {     /* Max connect time reached */
            printf("pppLinkStatusCallback: PPPERR_CONNECTTIME\n");
            break;
        }
        case PPPERR_LOOPBACK:
        {        /* Loopback detected */
            printf("pppLinkStatusCallback: PPPERR_LOOPBACK\n");
            break;
        }
        default:
        {
            printf("pppLinkStatusCallback: unknown errCode %d\n", errCode);
            break;
        }
    }
}

#if PPPOS_SUPPORT

static u32_t ppp_output_cb(ppp_pcb *pcb, u8_t *data, u32_t len, void *ctx)
{
    LWIP_UNUSED_ARG(pcb);
    LWIP_UNUSED_ARG(ctx);
    return sio_write(ppp_sio, data, len);
}

#endif /* PPPOS_SUPPORT */
#endif /* USE_PPP */

#if LWIP_NETIF_STATUS_CALLBACK

ip4_addr_t ourAddr = {0};
ip4_addr_t hisAddr = {0};

u_long ADAPTER_ADDR = 0;
u_long ADAPTER_MASK = 0;
u_long ADAPTER_GW = 0;
u_long ADAPTER_DNS = 0;

static void status_callback(struct netif *state_netif)
{
    if (netif_is_up(state_netif))
    {
#if LWIP_IPV4
        printf("status_callback==UP, local interface IP is %s\n", ip4addr_ntoa(netif_ip4_addr(state_netif)));
///

#if LWIP_USING_NAT
        printf("\n\n\n!!!NAT!!!\n\n\n");

        nat_entry.out_if = (struct netif *) &netif;
        nat_entry.in_if = (struct netif *) &ppp_netif;
        nat_entry.source_net = hisAddr;
        nat_entry.dest_net.addr = ADAPTER_ADDR;
        nat_entry.dest_netmask.addr = ADAPTER_MASK;
        IP4_ADDR(&nat_entry.source_netmask, 255, 255, 255, 0);
        printf("nat_entry.source_net.addr = %u\n",  nat_entry.source_net.addr);
        printf("nat_entry.dest_net.addr = %u\n",  nat_entry.dest_net.addr);
        printf("nat_entry.dest_netmask.addr = %u\n", nat_entry.dest_netmask.addr);
        printf("nat_entry.source_netmask.addr = %u\n", nat_entry.source_netmask.addr);
        ip_nat_add(&nat_entry);
#endif

///
#else

        printf("status_callback==UP\n");
#endif
#if LWIP_MDNS_RESPONDER
        mdns_resp_netif_settings_changed(state_netif);
#endif
    }
    else
    {
        printf("status_callback==DOWN\n");

#if LWIP_USING_NAT
        ip_nat_remove(&nat_entry);
#endif

    }
}

#endif /* LWIP_NETIF_STATUS_CALLBACK */

#if LWIP_NETIF_LINK_CALLBACK

static void link_callback(struct netif *state_netif)
{
    if (netif_is_link_up(state_netif))
    {
        printf("link_callback==UP\n");
    }
    else
    {
        printf("link_callback==DOWN\n");
    }
}

#endif /* LWIP_NETIF_LINK_CALLBACK */



/* This function initializes all network interfaces */
static void msvc_netif_init(void)
{
//#if LWIP_IPV4 && USE_ETHERNET
    ip4_addr_t ipaddr, netmask, gw;
//#endif /* LWIP_IPV4 && USE_ETHERNET */
//#if USE_SLIPIF
//    u8_t num_slip1 = 0;
//#if LWIP_IPV4
//  ip4_addr_t ipaddr_slip1, netmask_slip1, gw_slip1;
//#endif
//#if USE_SLIPIF > 1
//  u8_t num_slip2 = 1;
//#if LWIP_IPV4
//  ip4_addr_t ipaddr_slip2, netmask_slip2, gw_slip2;
//#endif
//#endif /* USE_SLIPIF > 1 */
//#endif /* USE_SLIPIF */
//#if USE_DHCP || USE_AUTOIP
//    err_t err;
//#endif

//#if USE_PPP
//    const char *username = NULL, *password = NULL;
//#ifdef PPP_USERNAME
//    username = PPP_USERNAME;
//#endif
//#ifdef PPP_PASSWORD
//    password = PPP_PASSWORD;
//#endif
    if (!callClosePpp)
    {

        printf("ppp_connect: COM%d\n", (int) sio_idx);
//#if PPPOS_SUPPORT
        ppp_sio = sio_open(sio_idx);
        if (ppp_sio == NULL)
        {
            printf("sio_open error\n");
        }
        else
        {
            /* Initiate PPP client connection. */
//            ppp = pppos_create(&ppp_netif, ppp_output_cb, pppLinkStatusCallback, NULL);
            ppp = pppapi_pppos_create(&ppp_netif, ppp_output_cb, pppLinkStatusCallback, NULL);
            if (ppp == NULL)
            {
                printf("pppos_create error\n");
            }

            /* Initiate PPP server connection. */
            ip4_addr_t addr;

            /* Set our address */
            ppp_set_ipcp_ouraddr(ppp, &ourAddr);

            /* Set peer(his) address */
            ppp_set_ipcp_hisaddr(ppp, &hisAddr);

            /* Set primary DNS server */
            if (ADAPTER_DNS)
            {
                addr.addr = ADAPTER_DNS;
            }
            else
            {
                IP4_ADDR(&addr, 8, 8, 8, 8);
            }
            ppp_set_ipcp_dnsaddr(ppp, 0, &addr);

            /* Set secondary DNS server */

            ppp_set_ipcp_dnsaddr(ppp, 1, &addr);

            /* Auth configuration, this is pretty self-explanatory */
//        ppp_set_auth(ppp, PPPAUTHTYPE_ANY, "login", "password");

            /* Require peer to authenticate */
//        ppp_set_auth_required(ppp, 1);

/*
 * Only for PPPoS, the PPP session should be up and waiting for input.
 *
 * Note: for PPPoS, ppp_connect() and ppp_listen() are actually the same thing.
 * The listen call is meant for future support of PPPoE and PPPoL2TP server
 * mode, where we will need to negotiate the incoming PPPoE session or L2TP
 * session before initiating PPP itself. We need this call because there is
 * two passive modes for PPPoS, ppp_set_passive and ppp_set_silent.
 */
            ppp_set_silent(ppp, 1);

/*
 * Initiate PPP listener (i.e. wait for an incoming connection), can only
 * be called if PPP session is in the dead state (i.e. disconnected).
 */
            ppp_listen(ppp);

        }
    }
//#endif /* PPPOS_SUPPORT */
//#endif  /* USE_PPP */

//#if USE_ETHERNET
//#if LWIP_IPV4
    if (!isIpInterfaceInitialized)
    {

#define NETIF_ADDRS &ipaddr, &netmask, &gw,
        ip4_addr_set_zero(&gw);
        ip4_addr_set_zero(&ipaddr);
        ip4_addr_set_zero(&netmask);
//#if USE_ETHERNET_TCPIP
//#if USE_DHCP
//    printf("Starting lwIP, local interface IP is dhcp-enabled\n");
//#elif USE_AUTOIP
//    printf("Starting lwIP, local interface IP is autoip-enabled\n");
//#else /* USE_DHCP */
        gw.addr = ADAPTER_GW;
        ipaddr.addr = ADAPTER_ADDR;
        netmask.addr = ADAPTER_MASK;
        printf("Starting lwIP, local interface IP is %s\n", ip4addr_ntoa(&ipaddr));
//#endif /* USE_DHCP */
//#endif /* USE_ETHERNET_TCPIP */
//#else /* LWIP_IPV4 */
//#define NETIF_ADDRS
//    printf("Starting lwIP, IPv4 disable\n");
//#endif /* LWIP_IPV4 */

//#if NO_SYS
//    netif_set_default(netif_add(&netif, NETIF_ADDRS NULL, pcapif_init, netif_input));
//#else  /* NO_SYS */
        netif_set_default(netif_add(&netif, NETIF_ADDRS NULL, pcapif_init, tcpip_input));
//#endif /* NO_SYS */
//#if LWIP_IPV6
//    netif_create_ip6_linklocal_address(&netif, 1);
//    printf("ip6 linklocal address: ");
//    ip6_addr_debug_print(0xFFFFFFFF & ~LWIP_DBG_HALT, netif_ip6_addr(&netif, 0));
//    printf("\n");
//#endif /* LWIP_IPV6 */
//#if LWIP_NETIF_STATUS_CALLBACK
        netif_set_status_callback(&netif, status_callback);
//#endif /* LWIP_NETIF_STATUS_CALLBACK */
//#if LWIP_NETIF_LINK_CALLBACK
        netif_set_link_callback(&netif, link_callback);
//#endif /* LWIP_NETIF_LINK_CALLBACK */

//#if USE_ETHERNET_TCPIP
//#if LWIP_AUTOIP
//    autoip_set_struct(&netif, &netif_autoip);
//#endif /* LWIP_AUTOIP */
//#if LWIP_DHCP
//    dhcp_set_struct(&netif, &netif_dhcp);
//#endif /* LWIP_DHCP */
        netif_set_up(&netif);
//#if USE_DHCP
//    err = dhcp_start(&netif);
//    LWIP_ASSERT("dhcp_start failed", err == ERR_OK);
//#elif USE_AUTOIP
//    err = autoip_start(&netif);
//  LWIP_ASSERT("autoip_start failed", err == ERR_OK);
//#endif /* USE_DHCP */
//#else /* USE_ETHERNET_TCPIP */
//    /* Use ethernet for PPPoE only */
//  netif.flags &= ~(NETIF_FLAG_ETHARP | NETIF_FLAG_IGMP); /* no ARP */
//  netif.flags |= NETIF_FLAG_ETHERNET; /* but pure ethernet */
//#endif /* USE_ETHERNET_TCPIP */

//#if USE_PPP && PPPOE_SUPPORT
//    /* start PPPoE after ethernet netif is added! */
//    ppp = pppoe_create(&ppp_netif, &netif, NULL, NULL, pppLinkStatusCallback, NULL);
//    if (ppp == NULL)
//    {
//        printf("pppos_create error\n");
//    }
//    else
//    {
//        ppp_set_auth(ppp, PPPAUTHTYPE_ANY, username, password);
//        ppp_connect(ppp, 0);
//    }
//#endif /* USE_PPP && PPPOE_SUPPORT */

//#endif /* USE_ETHERNET */
//#if USE_SLIPIF
//#if LWIP_IPV4
//#define SLIP1_ADDRS &ipaddr_slip1, &netmask_slip1, &gw_slip1,
//    LWIP_PORT_INIT_SLIP1_IPADDR(&ipaddr_slip1);
//    LWIP_PORT_INIT_SLIP1_GW(&gw_slip1);
//    LWIP_PORT_INIT_SLIP1_NETMASK(&netmask_slip1);
//    printf("Starting lwIP slipif, local interface IP is %s\n", ip4addr_ntoa(&ipaddr_slip1));
//#else
//#define SLIP1_ADDRS
//    printf("Starting lwIP slipif\n");
//#endif
//#if defined(SIO_USE_COMPORT) && SIO_USE_COMPORT
//    num_slip1++; /* COM ports cannot be 0-based */
//#endif
//    netif_add(&slipif1, SLIP1_ADDRS &num_slip1, slipif_init, ip_input);
//#if !USE_ETHERNET
//    netif_set_default(&slipif1);
//#endif /* !USE_ETHERNET */
//#if LWIP_IPV6
//    netif_create_ip6_linklocal_address(&slipif1, 1);
//    printf("SLIP ip6 linklocal address: ");
//    ip6_addr_debug_print(0xFFFFFFFF & ~LWIP_DBG_HALT, netif_ip6_addr(&slipif1, 0));
//    printf("\n");
//#endif /* LWIP_IPV6 */
//#if LWIP_NETIF_STATUS_CALLBACK
//    netif_set_status_callback(&slipif1, status_callback);
//#endif /* LWIP_NETIF_STATUS_CALLBACK */
//#if LWIP_NETIF_LINK_CALLBACK
//    netif_set_link_callback(&slipif1, link_callback);
//#endif /* LWIP_NETIF_LINK_CALLBACK */
//    netif_set_up(&slipif1);
//
//#if USE_SLIPIF > 1
//#if LWIP_IPV4
//#define SLIP2_ADDRS &ipaddr_slip2, &netmask_slip2, &gw_slip2,
//    LWIP_PORT_INIT_SLIP2_IPADDR(&ipaddr_slip2);
//    LWIP_PORT_INIT_SLIP2_GW(&gw_slip2);
//    LWIP_PORT_INIT_SLIP2_NETMASK(&netmask_slip2);
//    printf("Starting lwIP SLIP if #2, local interface IP is %s\n", ip4addr_ntoa(&ipaddr_slip2));
//#else
//#define SLIP2_ADDRS
//    printf("Starting lwIP SLIP if #2\n");
//#endif
//#if defined(SIO_USE_COMPORT) && SIO_USE_COMPORT
//    num_slip2++; /* COM ports cannot be 0-based */
//#endif
//    netif_add(&slipif2, SLIP2_ADDRS &num_slip2, slipif_init, ip_input);
//#if LWIP_IPV6
//    netif_create_ip6_linklocal_address(&slipif1, 1);
//    printf("SLIP2 ip6 linklocal address: ");
//    ip6_addr_debug_print(0xFFFFFFFF & ~LWIP_DBG_HALT, netif_ip6_addr(&slipif2, 0));
//    printf("\n");
//#endif /* LWIP_IPV6 */
//#if LWIP_NETIF_STATUS_CALLBACK
//    netif_set_status_callback(&slipif2, status_callback);
//#endif /* LWIP_NETIF_STATUS_CALLBACK */
//#if LWIP_NETIF_LINK_CALLBACK
//    netif_set_link_callback(&slipif2, link_callback);
//#endif /* LWIP_NETIF_LINK_CALLBACK */
//    netif_set_up(&slipif2);
//#endif /* USE_SLIPIF > 1*/
//#endif /* USE_SLIPIF */
        isIpInterfaceInitialized = true;
    }
}

#if LWIP_DNS_APP && LWIP_DNS
static void
dns_found(const char *name, const ip_addr_t *addr, void *arg)
{
  LWIP_UNUSED_ARG(arg);
  printf("%s: %s\n", name, addr ? ipaddr_ntoa(addr) : "<not found>");
}

static void
dns_dorequest(void *arg)
{
  const char* dnsname = "3com.com";
  ip_addr_t dnsresp;
  LWIP_UNUSED_ARG(arg);

  if (dns_gethostbyname(dnsname, &dnsresp, dns_found, 0) == ERR_OK) {
    dns_found(dnsname, &dnsresp, 0);
  }
}
#endif /* LWIP_DNS_APP && LWIP_DNS */

#if LWIP_LWIPERF_APP
static void
lwiperf_report(void *arg, enum lwiperf_report_type report_type,
  const ip_addr_t* local_addr, u16_t local_port, const ip_addr_t* remote_addr, u16_t remote_port,
  u32_t bytes_transferred, u32_t ms_duration, u32_t bandwidth_kbitpsec)
{
  LWIP_UNUSED_ARG(arg);
  LWIP_UNUSED_ARG(local_addr);
  LWIP_UNUSED_ARG(local_port);

  printf("IPERF report: type=%d, remote: %s:%d, total bytes: %lu, duration in ms: %lu, kbits/s: %lu\n",
    (int)report_type, ipaddr_ntoa(remote_addr), (int)remote_port, bytes_transferred, ms_duration, bandwidth_kbitpsec);
}
#endif /* LWIP_LWIPERF_APP */

#if LWIP_MDNS_RESPONDER

static void srv_txt(struct mdns_service *service, void *txt_userdata)
{
    err_t res = mdns_resp_add_service_txtitem(service, "path=/", 6);
    LWIP_ERROR("mdns add service txt failed\n", (res == ERR_OK), return);
    LWIP_UNUSED_ARG(txt_userdata);
}

#endif

/* This function initializes applications */
static void apps_init(void)
{
//#if LWIP_DNS_APP && LWIP_DNS
//    /* wait until the netif is up (for dhcp, autoip or ppp) */
//  sys_timeout(5000, dns_dorequest, NULL);
//#endif /* LWIP_DNS_APP && LWIP_DNS */
//
//#if LWIP_CHARGEN_APP && LWIP_SOCKET
//    chargen_init();
//#endif /* LWIP_CHARGEN_APP && LWIP_SOCKET */
//
//#if LWIP_PING_APP && LWIP_RAW && LWIP_ICMP
//    ping_init();
//#endif /* LWIP_PING_APP && LWIP_RAW && LWIP_ICMP */
//
//#if LWIP_NETBIOS_APP && LWIP_UDP
//    netbiosns_init();
//#ifndef NETBIOS_LWIP_NAME
//#if LWIP_NETIF_HOSTNAME
//  netbiosns_set_name(netif_default->hostname);
//#else
//  netbiosns_set_name("NETBIOSLWIPDEV");
//#endif
//#endif
//#endif /* LWIP_NETBIOS_APP && LWIP_UDP */
//
//#if LWIP_HTTPD_APP && LWIP_TCP
//#ifdef LWIP_HTTPD_APP_NETCONN
//    http_server_netconn_init();
//#else /* LWIP_HTTPD_APP_NETCONN */
//    httpd_init();
//#endif /* LWIP_HTTPD_APP_NETCONN */
//#endif /* LWIP_HTTPD_APP && LWIP_TCP */

//#if LWIP_MDNS_RESPONDER
    mdns_resp_init();
//#if LWIP_NETIF_HOSTNAME
//    mdns_resp_add_netif(netif_default, netif_default->hostname, 3600);
//#else
    mdns_resp_add_netif(netif_default, "lwip", 3600);
//#endif
    mdns_resp_add_service(netif_default, "lwipweb", "_http", DNSSD_PROTO_TCP, HTTPD_SERVER_PORT, 3600, srv_txt, NULL);
//#endif

//#if LWIP_NETIO_APP && LWIP_TCP
//    netio_init();
//#endif /* LWIP_NETIO_APP && LWIP_TCP */
//
//#if LWIP_RTP_APP && LWIP_SOCKET && LWIP_IGMP
//    rtp_init();
//#endif /* LWIP_RTP_APP && LWIP_SOCKET && LWIP_IGMP */
//
//#if LWIP_SNTP_APP && LWIP_SOCKET
//    sntp_init();
//#endif /* LWIP_SNTP_APP && LWIP_SOCKET */
//
//#if LWIP_SHELL_APP && LWIP_NETCONN
//    shell_init();
//#endif /* LWIP_SHELL_APP && LWIP_NETCONN */
//#if LWIP_TCPECHO_APP
//#if LWIP_NETCONN && defined(LWIP_TCPECHO_APP_NETCONN)
//    tcpecho_init();
//#else /* LWIP_NETCONN && defined(LWIP_TCPECHO_APP_NETCONN) */
//    tcpecho_raw_init();
//#endif
//#endif /* LWIP_TCPECHO_APP && LWIP_NETCONN */
//#if LWIP_UDPECHO_APP && LWIP_NETCONN
//    udpecho_init();
//#endif /* LWIP_UDPECHO_APP && LWIP_NETCONN */
//#if LWIP_LWIPERF_APP
//    lwiperf_start_tcp_server_default(lwiperf_report, NULL);
//#endif
//#if LWIP_SOCKET_EXAMPLES_APP && LWIP_SOCKET
//    socket_examples_init();
//#endif /* LWIP_SOCKET_EXAMPLES_APP && LWIP_SOCKET */
//#ifdef LWIP_APP_INIT
//    LWIP_APP_INIT();
//#endif
}

/* This function initializes this lwIP test. When NO_SYS=1, this is done in
 * the main_loop context (there is no other one), when NO_SYS=0, this is done
 * in the tcpip_thread context */
static void test_init(void *arg)
{ /* remove compiler warning */
    sys_sem_t *init_sem;
    LWIP_ASSERT("arg != NULL", arg != NULL);
    init_sem = (sys_sem_t *) arg;

    /* init randomizer again (seed per thread) */
    srand((unsigned int) time(0));

//    /* init network interfaces */
//    msvc_netif_init();

    /* init apps */
//    apps_init();

    sys_sem_signal(init_sem);
}

void lwipInit()
{
    if (!isLwipInitialized)
    {

        err_t err = 0;
        sys_sem_t init_sem = {0};
        /* initialize lwIP stack, network interfaces and applications */
        err = sys_sem_new(&init_sem, 0);
        LWIP_ASSERT("failed to create init_sem", err == ERR_OK);
        LWIP_UNUSED_ARG(err);
        tcpip_init(test_init, &init_sem);
        /* we have to wait for initialization to finish before
         * calling update_adapter()! */
        sys_sem_wait(&init_sem);
        sys_sem_free(&init_sem);

        isLwipInitialized = true;
    }

    /* init network interfaces */
    msvc_netif_init();
}

/* This is somewhat different to other ports: we have a main loop here:
 * a dedicated task that waits for packets to arrive. This would normally be
 * done from interrupt context with embedded hardware, but we don't get an
 * interrupt in windows for that :-) */
void lwipLoop(void)
{

    int count = 0;
    u8_t rxbuf[1024] = {0};

    lwipInit();

    /* MAIN LOOP for driver update (and timers if NO_SYS) */
    while (true)
    {
        sys_msleep(50);

        /* try to read characters from serial line and pass them to PPPoS */
        count = sio_read(ppp_sio, (u8_t *) rxbuf, 1024);
        if (count > 0)
        {
//            pppos_input(ppp, rxbuf, count);
            pppos_input_tcpip(ppp, rxbuf, count);
        }
        else
        {
            /* nothing received, give other tasks a chance to run */
            sys_msleep(1);
        }

        if (callClosePpp && ppp)
        {
            /* make sure to disconnect PPP before stopping the program... */
            callClosePpp = false;
            pppapi_close(ppp, 0);
            pppapi_free(ppp);
//            ppp_close(ppp, 0);
            puts("ppp closed.\n");

            ppp = NULL;

            break;
        }
    }

//    if (ppp)
//    {
//        u32_t started;
//        printf("Closing PPP connection...\n");
//        /* make sure to disconnect PPP before stopping the program... */
//        pppapi_close(ppp, 0);
//        pppapi_free(ppp);
////        ppp_close(ppp, 0);
//        ppp = NULL;
//        /* Wait for some time to let PPP finish... */
//        started = sys_now();
//        do
//        {
//            sys_msleep(50);
//            /* @todo: need a better check here: only wait until PPP is down */
//        }
//        while (sys_now() - started < 5000);
//    }
    /* release the pcap library... */

//    pcapif_shutdown(&netif);
//    netif_remove(&netif);

    puts("netif closed.\n");
}
