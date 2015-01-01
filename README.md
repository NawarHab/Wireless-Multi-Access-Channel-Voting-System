DESCRIPTION
===========

<img src="https://lh3.googleusercontent.com/-Lm9tx6z2f44/T_IAwTHJOoI/AAAAAAAAHtM/Ms3wbcFrrfQ/w1252-h939-no/DSCN1303.JPG" alt="voteino" width="50%">

* Arduino based portable multiuser voting system
* was tested with 4 voterremotes and 1 server


DOCUMENTATION
=============

For hardware and software configuration check https://github.com/silps/voteino/wiki/Configuration


INSTALLATION
============

* if you are using windows download the repositories in zip or 
* check https://github.com/msysgit/msysgit/wiki/InstallMSysGit to use git
* to use python in windows check http://docs.python.org/using/windows.html

1) install https://github.com/maniacbug/RF24/ library for arduino
2) download this repository
3) upload voteino.ino on your arduino
4) for using the python server we need to install django, python-serial and dajax

* unpack and install django
```bash
tar xzvf Django-1.4.tar.gz
cd Django-1.4
sudo python setup.py install && cd ..
```
* install python serial library
```bash
sudo apt-get install python-serial
```
* install django-dajax
```bash
git clone https://github.com/jorgebastida/django-dajax.git
cd django-dajax && sudo python setup.py install && cd ..
```
* install django-dajaxice
```bash
git clone https://github.com/jorgebastida/django-dajaxice.git
cd django-dajaxice && sudo python django-dajaxice/setup.py install && cd ..
```
5) once installed we can run the server
cd voteino/voteino_server && sudo python manage.py runserver 80
6) open browser and type localhost to see the client side, localhost/admin for admin side (user: test, pass: test)
7) to access the development server locally from another device use sudo python manage.py runserver 0.0.0.0:80


CREDITS ^-^
===========

* labmates and supervisors
* https://github.com/maniacbug/RF24/
* https://github.com/jorgebastida/django-dajax
* https://github.com/jorgebastida/django-dajaxice
* https://github.com/msysgit/msysgit/
