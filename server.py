
from flask import Flask
from flask import request
import os
from datetime import datetime
sessions = {}
app = Flask(__name__)

@app.route("/")
def hello():
    print(request.args.get("var"))
    return "We received value: " + str(request.args.get("var"))


@app.route("/soundbuddy")
def display():
	if(not request.args.get("session") and not request.args.get("value")):
		totalAvg = 0
		totalCount = 0

		result = "<h1>Soundbuddy overview</h1><br><h2>Sessions</h2><br>"
		for session in sessions:
			with open(session+".txt","r") as f:
				result += "<h3>Session " + session+" was created at " + str(sessions[session])+"</h3><br>"
				count = 0
				avg = 0
				for line in f:
					try:
						if(line.strip() == "inf"):
							continue
						print(float(line.strip()))
						avg+=float(line.strip())
						count+=1
					except:
						pass
				if(count == 0):
					result+= "No samples found on this date!<br>"
				else:
					print("avg",avg)
					print("count",count)
					result+="The average dB recorded was <b>" + str("{:.2f}".format(avg/count)) + "</b> with " + str(count) + " samples taken.<br><br><br>"
					totalAvg += avg/count
					totalCount+=1
		if(totalCount == 0):
			result+="No sessions found."

		else:
			result+="The average dB recorded across all sessions (" + str(totalCount) + ") was <b>"+str("{:.2f}".format(totalAvg/totalCount))+"dB."

		return result

	session = str(request.args.get("session"))
	if(session not in sessions):
		current_datetime = datetime.now()
		formatted_datetime = current_datetime.strftime("%Y-%m-%d %H:%M:%S")
		sessions[session] = formatted_datetime
	value = request.args.get("value")
	print(value)
	if(os.path.exists(session+".txt")):
		if(value):
			value = str(value)				
			with open(session+".txt","a") as f:
				f.write(value+"\r\n")
				return "Wrote " +value +" to session " + session
		else:

			with open(session+".txt","r") as f:
				result = "Session was created at " + str(sessions[session])+"<br>"
				count = 0
				avg = 0
				for line in f:
					try:
						if(line.strip() == "inf"):
							continue
						avg+=float(line.strip())
						count+=1
					except Exception as e:
						print(e)
				if(count == 0):
					result+= "No samples found on this date!"
				else:
					result+="The average dB recorded was <b>" + str("{:.2f}".format(avg/count)) + "</b> with " + str(count) + " samples taken."
				return result




	else:
		with open(session+".txt","w") as f:
			pass

		return "We received value: " + (session)
