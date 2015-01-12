#!/bin/sh

# aliases for broadcom tools
[ -x /usr/sbin/acsd ] && alias acsd='LD_LIBRARY_PATH=/lib/bcm acsd'
[ -x /usr/sbin/acs_cli ] && alias acs_cli='LD_LIBRARY_PATH=/lib/bcm acs_cli'
[ -x /usr/sbin/adslctl ] && alias adslctl='LD_LIBRARY_PATH=/lib/bcm adslctl'
[ -x /usr/sbin/arlctl ] && alias arlctl='LD_LIBRARY_PATH=/lib/bcm arlctl'
[ -x /usr/sbin/bpmctl ] && alias bpmctl='LD_LIBRARY_PATH=/lib/bcm bpmctl'
[ -x /usr/sbin/brctl ] && alias brctl='LD_LIBRARY_PATH=/lib/bcm brctl'
[ -x /usr/sbin/eapd ] && alias eapd='LD_LIBRARY_PATH=/lib/bcm eapd'
[ -x /usr/sbin/ebtables ] && alias ebtables='LD_LIBRARY_PATH=/lib/bcm ebtables'
[ -x /usr/sbin/ethswctl ] && alias ethswctl='LD_LIBRARY_PATH=/lib/bcm ethswctl'
[ -x /usr/sbin/fapctl ] && alias fapctl='LD_LIBRARY_PATH=/lib/bcm fapctl'
[ -x /usr/sbin/fcctl ] && alias fcctl='LD_LIBRARY_PATH=/lib/bcm fcctl'
[ -x /usr/sbin/gmacctl ] && alias gmacctl='LD_LIBRARY_PATH=/lib/bcm gmacctl'
[ -x /usr/sbin/hspotap ] && alias hspotap='LD_LIBRARY_PATH=/lib/bcm hspotap'
[ -x /usr/sbin/mcpctl ] && alias mcpctl='LD_LIBRARY_PATH=/lib/bcm mcpctl'
[ -x /usr/sbin/mcpd ] && alias mcpd='LD_LIBRARY_PATH=/lib/bcm mcpd'
[ -x /usr/sbin/nas ] && alias nas='LD_LIBRARY_PATH=/lib/bcm nas'
[ -x /usr/sbin/pwrctl ] && alias pwrctl='LD_LIBRARY_PATH=/lib/bcm pwrctl'
[ -x /usr/sbin/smd ] && alias smd='LD_LIBRARY_PATH=/lib/bcm smd'
[ -x /usr/sbin/swmdk ] && alias swmdk='LD_LIBRARY_PATH=/lib/bcm swmdk'
[ -x /usr/sbin/vlanctl ] && alias vlanctl='LD_LIBRARY_PATH=/lib/bcm vlanctl'
[ -x /usr/sbin/wlctl ] && alias wlctl='LD_LIBRARY_PATH=/lib/bcm wlctl'
[ -x /usr/sbin/wps_monitor ] && alias wps_monitor='LD_LIBRARY_PATH=/lib/bcm wps_monitor'
[ -x /usr/sbin/xdslctl ] && alias xdslctl='LD_LIBRARY_PATH=/lib/bcm xdslctl'
[ -x /usr/sbin/xtmctl ] && alias xtmctl='LD_LIBRARY_PATH=/lib/bcm xtmctl'