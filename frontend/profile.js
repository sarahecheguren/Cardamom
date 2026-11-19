const profileMessage = document.querySelector('#profile-message');
const profileName = document.querySelector('#profile-name');
const profileEmail = document.querySelector('#profile-email');
const profilePlan = document.querySelector('#profile-plan');
const profileRecipeCount = document.querySelector('#profile-recipe-count');
const profileUsageText = document.querySelector('#profile-usage-text');
const profileUsageBar = document.querySelector('#profile-usage-bar');

async function loadProfile() {
  profileMessage.textContent = 'Loading your account...';

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
      profileMessage.textContent = 'Unable to load your account. Please try again.';
      return;
    }

    const data = await response.json();
    const user = data.user;

    if (
      !user ||
      typeof user.display_name !== 'string' ||
      typeof user.email !== 'string' ||
      typeof user.tier_name !== 'string' ||
      !Number.isSafeInteger(user.recipe_count) ||
      user.recipe_count < 0 ||
      !Number.isSafeInteger(user.recipe_limit) ||
      user.recipe_limit <= 0
    ) {
      profileMessage.textContent = 'The server returned invalid account information.';
      return;
    }

    profileName.textContent = user.display_name;
    profileEmail.textContent = user.email;
    profilePlan.textContent = user.tier_name;
    profileRecipeCount.textContent = `${user.recipe_count} recipes saved`;
    profileUsageText.textContent = `${user.recipe_count} / ${user.recipe_limit} recipes`;

    profileUsageBar.max = user.recipe_limit;
    profileUsageBar.value = Math.min(user.recipe_count, user.recipe_limit);
    profileUsageBar.setAttribute(
      'aria-label',
      `${user.recipe_count} of ${user.recipe_limit} recipes used`
    );

    profileMessage.textContent = '';
  } catch (error) {
    profileMessage.textContent = 'Could not connect to the server. Please try again.';
  }
}

void loadProfile();
