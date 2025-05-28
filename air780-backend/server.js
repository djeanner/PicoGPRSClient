// server.js
const express = require('express');
const fs = require('fs');
const app = express();
const port = 3000;

let data = [];

app.use(express.urlencoded({ extended: true }));
app.use(express.json());

app.post('/submit', (req, res) => {
  const entry = {
    timestamp: new Date().toISOString(),
    ...req.body
  };
  data.push(entry);
  console.log('Data received:', entry);
  res.sendStatus(200);
});

app.get('/data', (req, res) => {
  res.json(data);
});

app.listen(port, () => {
  console.log(`Server running on http://localhost:${port}`);
});
