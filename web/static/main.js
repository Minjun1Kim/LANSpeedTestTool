const testTypeField = document.getElementById('test_type');
const numPacketsContainer = document.getElementById('num_packets_container');

function updateFieldVisibility() {
    const testType = testTypeField.value;
    if (testType === 'jitter') {
        numPacketsContainer.style.display = 'block';
    } else {
        numPacketsContainer.style.display = 'none';
    }
}

// Initial call to set visibility based on default selection
updateFieldVisibility();

// Listen for changes in test type
testTypeField.addEventListener('change', updateFieldVisibility);


const form = document.getElementById('testForm');
    let chart; 

    form.addEventListener('submit', async (e) => {
        e.preventDefault();

        const data = {
            test_type: document.getElementById('test_type').value,
            server_address: document.getElementById('server_address').value,
            size: parseInt(document.getElementById('size').value, 10),
            duration: parseInt(document.getElementById('duration').value, 10),
            num_packets: parseInt(document.getElementById('num_packets').value, 10)
        };

        // Send request to Flask server
        const response = await fetch('/start_test', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify(data)
        });

        const result = await response.json();
        document.getElementById('results').textContent = JSON.stringify(result, null, 0);

        if (result.status === 'success' && result.rtt_values) {
            const dataPoints = result.rtt_values;

            // If chart already exists, update it
            if (chart) {
                chart.data.labels = dataPoints.map((_, index) => index + 1);
                chart.data.datasets[0].data = dataPoints; 
                chart.update(); 
            } else {
                // Create a new chart if it doesn't exist
                chart = new Chart(document.getElementById('resultsChart'), {
                    type: 'line',
                    data: {
                        labels: dataPoints.map((_, index) => index + 1),
                        datasets: [{
                            label: 'RTT (microseconds)',
                            data: dataPoints, 
                            borderColor: 'blue',
                            fill: false
                        }]
                    },
                    options: {
                        responsive: true,
                        maintainAspectRatio: true,
                        scales: {
                            x: {
                                title: {
                                    display: true,
                                    text: 'Packet Number'
                                },
                                ticks: {
                                    autoSkip: false
                                }
                            },
                            y: {
                                title: {
                                    display: true,
                                    text: 'RTT (microseconds)'
                                },
                                beginAtZero: true
                            }
                        },
                        plugins: {
                            legend: {
                                display: true
                            }
                        }
                    }
                });
            }

            // Display jitter
            const existingJitterElement = document.getElementById('jitter');
            if (!existingJitterElement) {
                const newJitterElement = document.createElement('p');
                newJitterElement.id = 'jitter';
                newJitterElement.textContent = `Average Jitter: ${result.jitter} microseconds`;
                document.body.appendChild(newJitterElement);
            } else {
                existingJitterElement.textContent = `Average Jitter: ${result.jitter} microseconds`;
            }
        }
    }
);