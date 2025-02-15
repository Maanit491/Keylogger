const express = require('express');
const fs = require('fs');
const path = require('path');
const app = express();
const PORT = 4000;

// Serve static files from the "public" directory
app.use(express.static(path.join(__dirname, 'public')));

// Endpoint to fetch JSON data
app.get('/data', (req, res) => {
    const filePath = path.join(__dirname, 'keystrokes.json');
    fs.readFile(filePath, 'utf8', (err, data) => {
        if (err) {
            return res.status(500).send('Error reading JSON file');
        }
        res.json(JSON.parse(data));
    });
});

app.listen(PORT, () => {
    console.log(`Server is running on http://localhost:${PORT}`);
});
