<?
require("header.php");
require("menu.php");
?>
<h1>Domain Name Relay Daemon</h1>

<? menu("Resources"); ?>

<div id="content">

<p>Here are some DNS related links</p>

<ul>
<li><a href="http://dnrd.nevalabs.org">Brad Garcia's original page</a></li>

<li><a href="http://www.dns.net/dnsrd/rfc">DNS related rfc's</a></li>

<li><a href="http://freshmeat.net/projects/dnrd">Freshmeat page</a></li>
</ul>

<h3>Authorative DNS servers</h3>
<ul>

<li><a href="http://www.isc.org/products/BIND/bind9.html">Bind</a></li>


>     djbdns: http://cr.yp.to/djbdns.html
>     ldapdns: http://ldapdns.sourceforge.net/
>     maradns: http://www.maradns.org/
>     mydns: http://mydns.bboy.net/
>     powerdns: http://www.powerdns.com/
>     posadis: http://www.posadis.org/
>
> DNS proxies:
>     pdnsd (if you need disk cache):
> http://www.phys.uu.nl/%7Erombouts/pdnsd.html
>     dnsmasq (if you want integrated dhcp support):
> http://www.thekelleys.org.uk/dnsmasq/
>     dproxy (if you dont need domain "routing"):
> http://dproxy.sourceforge.net/
>     dnsproxy (if you don't need no cache):
> http://www.wolfermann.org/dnsproxy.html
> Otherwise, use DNRD :)


</div>

<? require("footer.php"); ?>
