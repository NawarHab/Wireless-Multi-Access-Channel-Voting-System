DESCRIPTION
===========
Lab project during masters degree
<img src="https://lh3.googleusercontent.com/J_2av7R-Mz8IiofyyfmqugvMVvBefm7BMVB8buKo75gKV049bfUAHR1X6X-0sTd926LYMO7Unt9gty9m0HQOH2S0VKtw2cL3ajzmw9BzAGVfKWkT57F4zLJM3Y1bH7aKvniHiE9Q7Gt3sgbEvU1WCaTOHFb7JSaLuPNQOgdTBA1iLswC-lst_WTBEX4NKHYiz1CvzI2zXWdrL-6_KNHWl1Jf_zZUgDdPPfYTJPaGV2_Ui2s9L565vLy-8w3fFwbtaX9i_5ILH7toFIXC1okxs9eJ4hIPcgq5DapG3tdK4nX2A3B3gYIAA0N_iJm3kv8OJYMdyP2EMXOFUD9TdqC93SyiYOE5sdwOjvH5ROEp3elusGsRUsUZTG4EDqSX2b-Tb7a6bmksuI-PZS-72erUVf05XtnQBJaakB323c68GlI9u0Z5Rgat9oS-9hdx4KzHWobNV7Hwd2cznlJKQ87RwfOk0rYbQw6FFCrbXqO0rU2YYQsGg8JA12MR5My2jFW4JOjqaLnEUAYwpdE4qha3Kd5NIs_2_9yBzDTVaDNlJEManJIhvEU_KZwBYDDP4q-E80MPuuq7vPaOwmXwId2yGBqL2_2gsnXKUgz06hzI=w936-h702-no" alt="voteino" width="50%">

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

1. install https://github.com/maniacbug/RF24/ library for arduino
2. download this repository
3. upload voteino.ino on your arduino
4. for using the python server we need to install django, python-serial and dajax
5. once installed we can run the server
cd voteino/voteino_server && sudo python manage.py runserver 80
6. open browser and type localhost to see the client side, localhost/admin for admin side (user: test, pass: test)
7. to access the development server locally from another device use sudo python manage.py runserver 0.0.0.0:80

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

CREDITS ^-^
===========

* labmates and supervisors
* https://github.com/maniacbug/RF24/
* https://github.com/jorgebastida/django-dajax
* https://github.com/jorgebastida/django-dajaxice
* https://github.com/msysgit/msysgit/
