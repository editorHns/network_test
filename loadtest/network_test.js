import http from 'k6/http';
import { check } from 'k6';

export const options = {
  scenarios: {
    network_test: {
      executor: 'constant-vus',
      vus: 20,
      duration: '30s',
    },
  },
  thresholds: {
    http_req_failed: ['rate<0.01'],
    http_req_duration: ['p(95)<500'],
  },
};

export default function () {
  const a = 10;
  const b = 20;
  const response = http.get(
    `http://127.0.0.1:8080/api/calc?a=${a}&b=${b}`
  );

  check(response, {
    'HTTP 200': (r) => r.status === 200,
    'result is 30': (r) => r.body.trim() === '30',
  });
}
