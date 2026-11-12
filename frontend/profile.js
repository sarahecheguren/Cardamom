const profileMessage = document.querySelector('#profile-message');
const profileName = document.querySelector('#profile-name');
const profileEmail = document.querySelector('#profile-email');

async function loadProfile() {
  profileMessage.textContent = 'Checking your account...';

  try {
    const response = await fetch('/api/auth/me', {
      credentials: 'same-origin',
      cache: 'no-store',
    });

    if (response.status === 401) {
      window.location.replace('index.html');
      return;
    }

    if (!response.ok) {
      profileMessage.textContent = 'Unable to check your account. Please try again.';
      return;
    }

    const data = await response.json();
    const user = data.user || {};

    // Display account details if the backend provides them.
    if (typeof user.display_name === 'string' && user.display_name) {
      profileName.textContent = user.display_name;
    }

    if (typeof user.email === 'string' && user.email) {
      profileEmail.textContent = user.email;
    }

    profileMessage.textContent =
      'You are signed in. Additional account details will be available after backend integration.';
  } catch (error) {
    profileMessage.textContent = 'Could not connect to the server. Please try again.';
  }
}

void loadProfile();
