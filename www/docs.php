<?
require("header.php");
require("menu.php");
?>
<h1>Domain Name Relay Daemon</h1>

<? menu("Manual Page"); ?>

<div id="content">


<h2>Manual Page</h2>
<A NAME="lbAB">&nbsp;</A>
<H3>NAME</H3>

dnrd - proxy name server
<A NAME="lbAC">&nbsp;</A>
<H3>SYNOPSIS</H3>



<DL COMPACT>
<DT><B>dnrd</B>

<DD>
[<B>-a </B><I>localaddress</I><B></B>|<B>--address=</B><I>localaddress</I><B></B>]

[<B>-c</B>&nbsp;(<B>off</B>|[<B></B><I>low</I>:]<B></B><I>high</I>)|<B>--cache=</B>(<B>off</B>|[<B></B><I>low</I>:]<B></B><I>high</I>)]

[<B>-b</B>|<B>--load-balance</B>]

[<B>-d</B>|<B>--debug</B>]

[<B>-h</B>|<B>--help</B>]

[<B>-k</B>|<B>--kill</B>]

[<B>-m</B>&nbsp;(<B>off</B>|<B>hosts</B>)|<B>--master=</B>(<B>off</B>|<B>hosts</B>)]

[<B>-s&nbsp;</B><I>ipaddr</I>(:<B></B><I>domain</I>)|<B>--server=</B><I>ipaddr</I>(:<B></B><I>domain</I>)]

[<B>-u&nbsp;</B><I>userid</I>|<B>--uid=</B><I>userid</I>]

[<B>-v</B>|<B>--version</B>]

</DL>
<A NAME="lbAD">&nbsp;</A>
<H3>DESCRIPTION</H3>

<B>dnrd</B> is a proxying nameserver. It forwards DNS queries to the appropriate

nameserver, but can also act as the primary nameserver for a few machines.
Proxying is configured on the command line using the 
<B>-s</B>

option.  By default,
dnrd will act as the primary nameserver for hosts found in
<I>/etc/hosts</I>.

<P>
<A NAME="lbAE">&nbsp;</A>
<H3>OPTIONS</H3>

<DL COMPACT>
<DT><B>-a</B>

<DD>
<DT><B>--address</B>

<DD>
Bind only to the interface with the specified address.  This is a security
feature.
<P>
<DT><B>-b</B>

<DD>
<DT><B>--load-balance</B>

<DD>
If several forward server is specified with -s the requests are load
balanced among the servers in a round robin scheme.
<P>
<DT><B>-c</B>&nbsp;(<B>off</B>|[<B></B><I>low</I>:]<B></B><I>high</I>)

<DD>
<DT><B>--cache=</B>(<B>off</B>|[<B></B><I>low</I>:]<B></B><I>high</I>)

<DD>
This option can be used to either turn off caching of DNS responses, or to
change the high and low watermarks.  The high water mark is the number of
cached DNS responses that will be kept before we start dropping entries.
The low water mark is the number of entries to keep around after we've
started dropping entries.  By default, caching is on, with low and high
watermarks of 800 and 1000 respectively.
<P>
<DT><B>-d</B>

<DD>
<DT><B>--debug</B>

<DD>
This turns on debugging.  The dnrd process will run in the current console
and print out debugging information.  If the option is given twice, it will
also dump out the raw packet contents.
<P>
<DT><B>-h</B>

<DD>
<DT><B>--help</B>

<DD>
Prints out usage information
<P>
<DT><B>-k</B>

<DD>
<DT><B>--kill</B>

<DD>
Kills the currently running dnrd process and immediately returns.
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

This option allows you to override this default behavior.  Setting it to
<B>off</B>

turns off all primary server functionality.  Setting it to 
<B>hosts</B>

causes dnrd to act as the primary server for hosts in
<I>/etc/hosts</I>

regardless of whether it could find
<I>/etc/dnrd/master</I>.

<P>
<DT><B>-s&nbsp;</B><I>ipaddr</I><B></B>(:<I>domain</I><B></B>)

<DD>
<DT><B>--server=</B><I>ipaddr</I><B></B>(:<I>domain</I><B></B>)

<DD>
This option is used to tell dnrd where to forward DNS queries.  If multiple
<B>-s</B>

options are given with no domain names, then  dnrd
will treat these as redundant DNS servers, and will switch between them
whenever the current one stops responding.
<P>
The 
<I>domain</I>

option allows dnrd
to determine which DNS server should get the query based on the domain name
in the query.  This is useful when you have an internet connection and a
vpn connection to work, for instance.  In this case, all the
<B>-s</B>

options should have a domain defined, except for the last one, which will act
as the &quot;default&quot; name server.
<P>
<DT><B>-u&nbsp;</B><I>userid</I>

<DD>
<DT><B>--userid=</B><I>userid</I>

<DD>
By default, dnrd switches to uid 65535 after starting up.  This is a
security feature. If dnrd is ever compromised by a buffer-overflow attack,
the attacker will have gained no useful privileges.  The default uid can
be overridden using this option.
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
<H3>BUGS</H3>

<P>

Even if it is possible to bind dnrd to one single interface it is not
possible to run several dnrd processes on the same host.
<P>
<A NAME="lbAG">&nbsp;</A>
<H3>FILES</H3>

<P>

<B>/etc/dnrd/master</B>

<P>

This file is used to configure
<B>dnrd</B>

as a primary nameserver.
<P>

<B>/etc/hosts</B>

<P>

By default, dnrd will act as a primary nameserver for hosts found in this file.
<P>

<B>/var/run/dnrd.pid</B>

<P>

The currently-running dnrd process' pid is placed into this file.
It is needed to allow new dnrd processes to find and kill the currently
running process.
<P>
<A NAME="lbAH">&nbsp;</A>
<H3>AUTHOR</H3>

<P>

The original version of dnrd was written by Brad Garcia
<B><A HREF="mailto:garsh@home.com">garsh@home.com</A></B>.

Other contributors are listed in the HISTORY
file included with the source code.
<P>

</div>

<? require("footer.php"); ?>
