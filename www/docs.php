<?
require("header.php");
require("menu.php");
?>
<h1>Domain Name Relay Daemon</h1>

<? menu("Manual Page"); ?>

<div id="content">

<H2>DNRD</H2>


dnrd - proxy name server
<A NAME="lbAC">&nbsp;</A>
<h3>SYNOPSIS</h3>



<DL COMPACT>
<DT><B>dnrd</B>

<DD>
[<B>-a</B>&nbsp;<I>localaddress</I><B>|</B>--address=<I>localaddress</I><B>]</B>

[<B>-b</B>|<B>--load-balance</B>]

[<B>-c</B>&nbsp;(<B>off</B>|[<B></B><I>low</I>:]<B></B><I>high</I>)|<B>--cache=</B>(<B>off</B>|[<B></B><I>low</I>:]<B></B><I>high</I>)]

[<B>-d</B>|<B>--debug</B>]

[<B>-h</B>|<B>--help</B>]

[<B>-i</B>|<B>--ignore</B>]

[<B>-k</B>|<B>--kill</B>]

[<B>-l</B>|<B>--log</B>]

[<B>-m</B>&nbsp;(<B>off</B>|<B>hosts</B>)|<B>--master=</B>(<B>off</B>|<B>hosts</B>)]

[<B>-M</B>&nbsp;<I>N</I><B>|</B>--max-sock=<I>N</I><B>]</B>

[<B>-r</B>&nbsp;<I>N</I><B>|</B>--retry=<I>N</I><B>]</B>

[<B>-s&nbsp;</B><I>ipaddr</I>(:<B></B><I>domain</I>)|<B>--server=</B><I>ipaddr</I>(:<B></B><I>domain</I>)]

[<B>-t</B>&nbsp;<I>N</I><B>|</B>--timeout=<I>N</I><B>]</B>

[<B>-u&nbsp;</B><I>userid</I>|<B>--uid=</B><I>userid</I>]

[<B>-v</B>|<B>--version</B>]

</DL>
<A NAME="lbAD">&nbsp;</A>
<h3>DESCRIPTION</h3>

<B>dnrd</B> is a proxying nameserver. It forwards DNS queries to the appropriate

nameserver, but can also act as the primary nameserver for a subnet
behind a firewall.  Proxying is configured on the command line using
the
<B>-s</B>

option.  By default, dnrd will act as the primary nameserver for hosts
found in
<I>/etc/hosts</I>.

<P>
<A NAME="lbAE">&nbsp;</A>
<h3>OPTIONS</h3>

<DL COMPACT>
<DT><B>-a</B>

<DD>
<DT><B>--address</B>

<DD>
Bind only to the interface with the specified address. By default dnrd
binds to everything.
<P>
<DT><B>-b</B>

<DD>
<DT><B>--load-balance</B>

<DD>
Turn on load balancing. All forward servers that are specified after
this option (with
<B>-s</B>)

will load balance in a round robin scheme. By default, dnrd will use
the next server in the list if the first times out. As soon as the
first is reactivated, it will be used again. With
<B>-b</B>

option, dnrd will use next active server as soon a request is
served. If a server times out it will be deactivated and will not be
used until it comes back. As soon it is reactivated it will join the
list.
<P>
Note that if there are no
<B>-s</B>

after the
<B>-b</B>,

this will do nothing at all.
<P>
<DT><B>-c</B>&nbsp;(<B>off</B>|[<B></B><I>low</I>:]<B></B><I>high</I>)

<DD>
<DT><B>--cache=</B>(<B>off</B>|[<B></B><I>low</I>:]<B></B><I>high</I>)

<DD>
This option can be used to either turn off caching of DNS responses,
or to change the high and low watermarks. With the high/low water mark
option, cached entries are purged when the number of responses reaches
the high-water mark, and they will be purged until the number of
cached responses reaches the low-water mark, purging the oldest
first. By default, caching is on, with low and high water-marks of 800
and 1000 respectively.
<P>
<DT><B>-d</B>

<DD>
<DT><B>--debug</B>

<DD>
This turns on debugging.  The dnrd process will not fork into the
background and print out debugging information in the current
console. If the option is given twice, it will also dumps the raw
packet contents.
<P>
<DT><B>-h</B>

<DD>
<DT><B>--help</B>

<DD>
Prints usage information
<P>
<DT><B>-i</B>

<DD>
<DT><B>--ignore</B>

<DD>
Ignore cache for deactivated servers. If a forward DNS server times
out and gets deactivated, all cache entries for this server are
ignored. This helps avoid network timeout delays when dnrd serves a
offline/dialup network.
<P>
<DT><B>-k</B>

<DD>
<DT><B>--kill</B>

<DD>
Kills the currently running dnrd process.
<P>
<DT><B>-l</B>

<DD>
<DT><B>--log</B>

<DD>
Send all messages to syslog. dnrd uses the deamon facility.
<P>
<DT><B>-m</B>&nbsp;(<B>off</B>|<B>hosts</B>)

<DD>
<DT><B>--master=</B>(<B>off</B>|<B>hosts</B>)

<DD>
dnrd can act as the primary name server for a number of hosts.  By default, it
will read in
<I>/etc/dnrd/master</I>

to determine how this is done.  If that file
doesn't exist, it will act as the primary server for the hosts found in
<I>/etc/hosts</I>.

This option allows you to override the default behavior.  Setting it to
<B>off</B>

turns off all primary server functionality.  Setting it to 
<B>hosts</B>

causes dnrd to act as the primary server for hosts in
<I>/etc/hosts</I>

regardless of whether it could find
<I>/etc/dnrd/master</I>.

<P>
<DT><B>-M&nbsp;</B><I>N</I>

<DD>
<DT><B>--max-sock=</B><I>N</I>

<DD>
Set the maximum allowed open sockets. Default is 200.
<P>
<DT><B>-r&nbsp;</B><I>N</I>

<DD>
<DT><B>--retry=</B><I>N</I>

<DD>
Set the retry interval time. When a forward DNS server times
out it is deactivated. (use the
<B>-t</B>

option to set the timeout value) Dnrd will try to send a request for
localhost every
<I>N</I>

seconds. As soon there are a respose from a deactivated server, it
is reactivated. The default value is
<I>10</I>

seconds. Setting this to zero will make dnrd to never deactivate a
server.
<P>
<DT><B>-s&nbsp;</B><I>ipaddr</I><B></B>(:<I>domain</I><B></B>)

<DD>
<DT><B>--server=</B><I>ipaddr</I><B></B>(:<I>domain</I><B></B>)

<DD>
Add a forward DNS server. If multiple
<B>-s</B>

options are given, dnrd treats the first as a primary DNS server and
the rest as backup servers. If the primary DNS server times out, it
is deactivated and the next specified server (that is active)
is used until the previous gets reactivated.
<P>
The 
<I>domain</I>

option allows dnrd to determine which DNS server should get the query
based on the domain name in the query. This is useful when you have
an internet connection and a vpn connection to work, for instance. Several servers with the same 
<I>domain</I>

might be specified and then will they work as backup servers. 
<P>
If
<B>-b</B>

option is specified, then will all servers specified after the
<B>-b</B>

option, be grouped by 
<I>domain</I>,

and load balanced.
<P>
<DT><B>-t&nbsp;</B><I>N</I>

<DD>
<DT><B>--timeout=</B><I>N</I>

<DD>
Set the timeout value for forward DNS servers. If a server don't
respond to a query within
<I>N</I>

seconds it is deactivated. The default value is
<I>20</I>

<P>
Setting this to zero will make dnrd to never deactivate a server
because of timeouts. However, a server might be deactivated if sendto
fails.
<P>
<DT><B>-u&nbsp;</B><I>userid</I>

<DD>
<DT><B>--userid=</B><I>userid</I>

<DD>
By default, dnrd switches to uid 65535 after starting up.  This is a
security feature.  The default uid can be overridden using this
option.
<I>userid</I>

can either be a name or a number.
<P>
<DT><B>-v</B>

<DD>
<DT><B>--version</B>

<DD>
Prints out the version number
<P>
</DL>
<A NAME="lbAF">&nbsp;</A>
<h3>BUGS</h3>

<P>

It is not possible to run more than one dnrd process on the same host.
<P>
<A NAME="lbAG">&nbsp;</A>
<h3>FILES</h3>

<P>

<B>/etc/dnrd/master</B>

<P>

This file is used to configure
<B>dnrd</B>

as a primary nameserver.
<P>

<B>/etc/hosts</B>

<P>

By default, dnrd will act as a primary nameserver for hosts found in
this file.
<P>

<B>/var/run/dnrd.pid</B>

<P>

The currently-running dnrd process' pid is placed into this file.
It is needed to allow new dnrd processes to find and kill the currently
running process.
<P>
<A NAME="lbAH">&nbsp;</A>
<h3>AUTHOR</h3>

<P>

The original version of dnrd was written by Brad Garcia
<B><A HREF="mailto:garsh@home.com">garsh@home.com</A></B>.

Other contributors are listed in the HISTORY
file included with the source code.
<P>


</div>

<? require("footer.php"); ?>
