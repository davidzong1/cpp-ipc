// udp_domain_filter.bpf.c
#include <bpf/bpf_endian.h>
#include <bpf/bpf_helpers.h>
#include <linux/bpf.h>
#include <linux/ip.h>
#include <linux/udp.h>

struct MsgHeader {
  __u16 magic;
  __u8 version;
  __u8 flags;
  __u16 domain_id;
  __u16 msg_type;
  __u32 body_len;
} __attribute__((packed));

#define MSG_MAGIC 0xD1A5
#define MSG_VERSION 1

struct {
  __uint(type, BPF_MAP_TYPE_HASH);
  __uint(max_entries, 1024);
  __type(key, __u64);   // socket cookie
  __type(value, __u16); // allowed domain id (host order)
} sock_domain_map SEC(".maps");

SEC("socket")
int udp_domain_filter(struct __sk_buff *skb) {
  struct iphdr iph;
  struct udphdr udph;
  struct MsgHeader hdr;
  __u64 cookie = bpf_get_socket_cookie(skb);
  __u16 *allowed = bpf_map_lookup_elem(&sock_domain_map, &cookie);

  if (!allowed) {
    return 0; // 未注册的 socket 直接丢包
  }

  if (bpf_skb_load_bytes(skb, 0, &iph, sizeof(iph)) < 0) {
    return 0;
  }
  if (iph.version != 4 || iph.protocol != IPPROTO_UDP) {
    return 0;
  }

  __u32 ip_hlen = iph.ihl * 4;
  if (bpf_skb_load_bytes(skb, ip_hlen, &udph, sizeof(udph)) < 0) {
    return 0;
  }

  __u32 payload_off = ip_hlen + sizeof(udph);
  if (bpf_skb_load_bytes(skb, payload_off, &hdr, sizeof(hdr)) < 0) {
    return 0;
  }

  if (bpf_ntohs(hdr.magic) != MSG_MAGIC) {
    return 0;
  }
  if (hdr.version != MSG_VERSION) {
    return 0;
  }

  __u16 domain = bpf_ntohs(hdr.domain_id);
  if (domain != *allowed) {
    return 0;
  }

  return skb->len; // 放行
}

char _license[] SEC("license") = "GPL";