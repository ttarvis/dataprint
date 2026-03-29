import json

with open("examples/1GB.jsonl", "w") as f:
    f.write(json.dumps(["Name", "Session", "Score", "Completed"]) + "\n")
    f.write(json.dumps(["Gilbert", "2013", 24, True]) + "\n")
    line = json.dumps(["Alexa", "2013", 29, True]) + "\n"
    for _ in range(35714285):
        f.write(line)
