#!/bin/bash
###############################################################################
# This script:
# 1.  Creates and modifies /var/empty as necessary.
# 2.  Creates a local user account that has minimal priveleges, but which
#     can run as a service.
# 3.  Removes /var/log/dnrd.log.  The service will recreate this file when
#     it runs.
# 4.  Installs dnrd as a Win32 service that starts automatically when the
#     computer is booted up.  
#
# To use this script, edit the next section "Configuration items."  In
# particular, the item dnrd_parameters will certainly have to be modified.  
# See the man page for dnrd(8) for details.
# Next, execute this script (no command line arguments).
# If a string beginning with "exiting" is seen,
# the script has failed.  If the script succeeds, it prints:
#     Configuration complete!
#
# After using this script, you should probably update your /etc/passwd file.
###############################################################################


###############################################################################
# Configuration items:  Customize these to suit.
###############################################################################

# Name for the user account to be created
user_name="dnrd"

# Description of the user account to be created
user_desc="Unpriveleged account to run dnrd as a service"

# Name of service that runs dnrd
service_name="dnrd"

# Description of service that runs dnrd
service_desc="dnrd: relay domain name server (DNS)"

# CYGWIN environment variable to set for the service that runs dnrd
cygwin_var="server ntea ntsec smbntsec"

# Command line for dnrd. 
dnrd_parameters="-l -c off -t 0 -s 192.168.0.1 -a 169.254.25.129"

# So far so good!
failure="no"

###############################################################################
# Set up /var/empty as necessary
###############################################################################
LOCALSTATEDIR=/var
if [ -f ${LOCALSTATEDIR}/empty ]
then
   echo "Exiting: ${LOCALSTATEDIR}/empty is a file"
   exit 1
fi
mkdir -p ${LOCALSTATEDIR}/empty || failure="yes"
chmod 755 ${LOCALSTATEDIR}/empty || failure="yes"
chown SYSTEM ${LOCALSTATEDIR}/empty || failure="yes"
if [ "$failure" == "yes" ]
then
   echo "Exiting: failed to set up ${LOCALSTATEDIR}/empty"
   exit 1
fi
echo "Set up ${LOCALSTATEDIR}/empty ..."


###############################################################################
# Create the user account
###############################################################################

net user "$user_name" >/dev/null 2>&1 && failure=yes
if [ "$failure" == "yes" ]
then
    echo "Exiting: user account $user_name already exists"
    exit 1
fi
dos_var_empty=`cygpath -w ${LOCALSTATEDIR}/empty`
echo "Please enter a password for new user ${user_name}:"
read -s _password
if [ -z "${_password}" ]
then
    echo "Exiting: could not obtain a password for new user account"
    exit 1
fi
net user "$user_name" "${_password}" /add /fullname:"$user_desc" /homedir:"$dos_var_empty" >/dev/null 2>&1 || failure="yes"
if [ "$failure" == "yes" ]
then
    echo "Exiting: failed to create new user account $user_name"
    exit 1
fi
net user "$user_name" >/dev/null 2>&1 || failure=yes
if [ "$failure" == "yes" ]
then
    echo "Exiting: failed to create user account $user_name"
    exit 1
fi
editrights -a SeServiceLogonRight -u "$user_name" > /dev/null 2>&1 || failure=yes
if [ "$failure" == "yes" ]
then
    echo "Exiting: failed to add privelege to new user account $user_name"
    exit 1
fi
echo "Created user account $user_name ..."


###############################################################################
# Create the service
###############################################################################

cygrunsrv -Q "$service_name" >/dev/null 2>&1 && failure=yes
if [ "$failure" == "yes" ]
then
    echo "Exiting: Service $service_name already exists"
    exit 1
fi
cygrunsrv \
	--install "$service_name" \
	--type auto \
	--path /usr/local/sbin/dnrd.exe \
	--args "$dnrd_parameters" \
	--desc "$service_desc" \
	--user "$user_name" \
	--passwd "${_password}" \
	--env "CYGWIN=$cygwin_var" \
	>/dev/null 2>&1 \
	|| failure=yes
if [ "$failure" == "yes" ]
then
    echo "Exiting: Could not create service $service_name"
    exit 1
fi
cygrunsrv -Q "$service_name" >/dev/null 2>&1 || failure=yes
if [ "$failure" == "yes" ]
then
    echo "Exiting: Could not create service $service_name"
    exit 1
fi
echo "Created service $service_name ..."
rm -f /var/log/dnrd.log
net start "$service_name"
echo "Started service $service_name ..."

echo " "
echo "Configuration complete!"
