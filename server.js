// server.js
const express = require("express");
const fs = require("fs");
const app = express();
const PORT = process.env.PORT || 3000;

let dataStored = [];

app.use(express.urlencoded({ extended: true }));
app.use(express.json());

app.post("/submit", (req, res) => {
	const data = req.body;
	const date = new Date().toISOString();
	const zurichTime = new Date().toLocaleString('sv-SE', {timeZone:'Europe/Zurich'});
	const entry = {
		timestamp: date,
		zuDate: zurichTime,
		...data,
	};
	if (entry.device) {
		dataStored.push(entry);
		console.log("Data received:", entry);
		console.log("dataStored size:", dataStored.length);
		res.sendStatus(200);
		return;
	}

	console.log("Unknown data type : ", entry.device);

	res.sendStatus(200);
});

app.get("/data", (req, res) => {
	const { device, index } = req.query; // for wget "http://localhost:3000/data?device=AIR780&index=2" -O response.json

	if (index !== undefined) {
		const i = parseInt(index, 10);
		if (isNaN(i) || i < 0 || i >= dataStored.length) {
			return res.status(404).json({ error: "Index out of bounds" });
		}
		const item = dataStored[i];
		return res.json(item);
	}

	if (device) {
		const filtered = dataStored.filter(
			(entry) => entry.device === device
		);
		dataStored = dataStored.filter(
			(entry) => entry.device !== device
		); // remove matched ones
		console.log(
			`Returned and removed ${filtered.length} entries for device: ${device}`
		);
		return res.json(filtered);
	}

	res.json(dataStored);
});


// Add this before app.listen
app.get('/', (req, res) => {
  res.send('📡 PicoGPRSClient is running. Use POST /submit or GET /data');
});

app.listen(PORT, '0.0.0.0', () => {
  console.log(`Server V0.2 running on http://0.0.0.0:${PORT}`);
});
