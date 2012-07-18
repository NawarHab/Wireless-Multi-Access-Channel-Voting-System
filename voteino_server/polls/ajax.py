from django.utils import simplejson
from dajaxice.core import dajaxice_functions
from polls.models import Poll, Choice
import serial, time

def getVotes(request, poll_id):
    error = False
    try:
	arduino = serial.Serial( "/dev/ttyACM0", 115200, timeout = 1 )
	text = arduino.readline().rstrip()
    except serial.SerialException:
	error = True
    else:
	text_array = text.split(',')
	try:
	    newVotes = map(lambda x: int(x), text_array)
	except ValueError:
	    error = True

    try:
	p = Poll.objects.get(pk=poll_id)
    except Poll.DoesNotExist:
	choices = ['some', 'error', 'happened', '^ ^', 'LOL']
    else:
	if not(error) and len(newVotes) >= 5:
	    i = 0
	    for choice in p.choice_set.all():
		choice.votes = newVotes[i]
		choice.save()
		i += 1
	votes = []
	choices = []
	for choice in p.choice_set.values_list():
	    choices.append(choice[2])
	    votes.append(choice[3])
    #dajax.assign('#result', 'value', ')
    return simplejson.dumps({'choices': choices, 'votes': votes})

dajaxice_functions.register(getVotes)
