// Handles the login page and the registration preview.
const form = document.querySelector('#auth-form');
const message = document.querySelector('#form-message');
const password = document.querySelector('#password');
const confirmation = document.querySelector('#confirm-password');
const email = document.querySelector('#email');
const displayName = document.querySelector('#display-name');
const fields = Array.from(form.querySelectorAll('.field input'));
const submitButton = form.querySelector('[type="submit"]');
const logoutButton = document.querySelector('#logout-button');
const isLoginPage = form.dataset.mode === 'login';


function showMessage(text, success = false) {
  message.textContent = text;
  message.classList.toggle('is-success', success);
  message.hidden = false;
}

function setError(field, text) {
  document.querySelector('#' + field.id + '-error').textContent = text;
  if (text) {
    field.setAttribute('aria-invalid', 'true');
  } else {
    field.removeAttribute('aria-invalid');
  }
}

function validateField(field) {
  let error = '';
  if (!field.value.trim()) {
    error = 'Please fill in this field.';
  } else if (!isLoginPage && field === password && field.value.length < 8) {
    error = 'Password must be at least 8 characters.';
  } else if (field === displayName && field.value.trim().length > 100) {
    error = 'Display name must be 100 characters or fewer.';
  } else if (field.type === 'email' && field.validity.typeMismatch) {
    error = 'Please enter a valid email address.';
  } else if (field === confirmation && field.value !== password.value) {
    error = 'Your passwords do not match.';
  }
  setError(field, error);
  return error === '';
}

function setLoggedIn(loggedIn) {
  if (!isLoginPage) return;
  form.querySelectorAll('.field, .checkbox-label').forEach((element) => {
    element.hidden = loggedIn;
  });
  submitButton.hidden = loggedIn;
  logoutButton.hidden = !loggedIn;
}

async function requestApi(path, options = {}) {
  const response = await fetch(path, {
    ...options,
    credentials: 'same-origin', // Drogon keeps login state in a session cookie.
  });
  const body = await response.json().catch(() => ({}));
  return { response, body };
}

fields.forEach((field) => {
  field.addEventListener('input', () => {
    message.hidden = true;
    if (field.hasAttribute('aria-invalid')) validateField(field);
    if (field === password && confirmation?.hasAttribute('aria-invalid')) {
      validateField(confirmation);
    }
  });
});

document.querySelector('#show-password').addEventListener('change', (event) => {
  const type = event.target.checked ? 'text' : 'password';
  password.type = type;
  if (confirmation) confirmation.type = type;
});

form.addEventListener('submit', async (event) => {
  event.preventDefault();
  message.hidden = true;
  const valid = fields.map(validateField).every(Boolean);
  if (!valid) {
    form.querySelector('[aria-invalid="true"]').focus();
    return;
  }

  if (!isLoginPage) {
    // The backend does not have a registration API yet.
    showMessage('Registration is not available yet. No account was created.');
    password.value = '';
    if (confirmation) confirmation.value = '';
    return;
  }

  submitButton.disabled = true;
  try {
    const { response, body } = await requestApi('/api/auth/login', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ email: email.value.trim(), password: password.value }),
    });

    if (!response.ok) {
      showMessage(body.error || 'Unable to log in. Please try again.');
      return;
    }

    setLoggedIn(true);
    showMessage('Logged in as ' + (body.user?.email || email.value.trim()) + '.', true);
  } catch (error) {
    showMessage(
      'Cannot reach the backend. Open the site through the Drogon server at http://localhost:8080.'
    );
  } finally {
    password.value = '';
    submitButton.disabled = false;
  }
});

if (isLoginPage) {
  logoutButton.addEventListener('click', async () => {
    logoutButton.disabled = true;
    try {
      const { response, body } = await requestApi('/api/auth/logout', { method: 'POST' });
      if (!response.ok) {
        showMessage(body.error || 'Could not log out.');
        return;
      }
      setLoggedIn(false);
      form.reset();
      password.type = 'password';
      showMessage('You have been logged out.', true);
    } catch (error) {
      showMessage('Cannot reach the backend to log out.');
    } finally {
      logoutButton.disabled = false;
    }
  });

  // The server remembers sessions, so check the login state on page reload.
  requestApi('/api/auth/me')
    .then(({ response, body }) => {
      if (response.ok && body.user?.id) {
        setLoggedIn(true);
        showMessage('You are already signed in (user ID: ' + body.user.id + ').', true);
      }
    })
    .catch(() => {
      // Keep the form usable if the backend is not running yet.
    });
}

// The form stays disabled if JavaScript failed to load.
submitButton.disabled = false;
