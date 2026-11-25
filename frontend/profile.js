const profileMessage = document.querySelector('#profile-message');
const profileName = document.querySelector('#profile-name');
const profileEmail = document.querySelector('#profile-email');
const profilePlan = document.querySelector('#profile-plan');
const profileRecipeCount = document.querySelector('#profile-recipe-count');
const profileUsageText = document.querySelector('#profile-usage-text');
const profileUsageBar = document.querySelector('#profile-usage-bar');
const profileUpgrade = document.querySelector('#profile-upgrade');

// Elementos do formulario
const accountDetails = document.querySelector('#account-details');
const editButton = document.querySelector('#edit-profile-button');
const editForm = document.querySelector('#edit-profile-form');
const displayNameInput = document.querySelector('#edit-display-name');
const emailInput = document.querySelector('#edit-email');
const passwordInput = document.querySelector('#current-password');
const passwordConfirmation = document.querySelector('#password-confirmation');
const cancelButton = document.querySelector('#cancel-edit-button');
const saveButton = document.querySelector('#save-profile-button');
const editMessage = document.querySelector('#edit-profile-message');

let originalName = '';
let originalEmail = '';
let saving = false;

const fields = {
  display_name: {
    input: displayNameInput,
    error: document.querySelector('#display-name-error'),
  },
  email: {
    input: emailInput,
    error: document.querySelector('#edit-email-error'),
  },
  current_password: {
    input: passwordInput,
    error: document.querySelector('#current-password-error'),
  },
};

// Limpa as mensagens de erro
function clearErrors() {
  Object.values(fields).forEach((field) => {
    field.error.textContent = '';
    field.input.removeAttribute('aria-invalid');
  });

  editMessage.textContent = '';
}

// Mostra o erro no campo correspondente
function showFieldError(fieldName, message) {
  const field = fields[fieldName];

  if (field) {
    field.error.textContent = message;
    field.input.setAttribute('aria-invalid', 'true');
  }

  editMessage.textContent = message;
}

// Atualiza as informacoes da conta
function displayAccount(user) {
  originalName = user.display_name;
  originalEmail = user.email;

  profileName.textContent = originalName;
  profileEmail.textContent = originalEmail;
}

// Carrega as informacoes do perfil
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

    if (typeof user.display_name === 'string' && typeof user.email === 'string') {
      displayAccount(user);
      editButton.disabled = false;
    }

    if (typeof user.tier === 'string' && user.tier) {
      profilePlan.textContent = user.tier;
    }

    // Apenas usuarios Free podem fazer upgrade
    profileUpgrade.hidden = user.tier !== 'Free';

    const count = user.recipe_count;
    const limit = user.recipe_limit;

    if (Number.isInteger(count)) {
      profileRecipeCount.textContent = count + (count === 1 ? ' recipe saved' : ' recipes saved');
    }

    if (Number.isInteger(count) && Number.isInteger(limit) && limit > 0) {
      profileUsageText.textContent = count + ' / ' + limit + ' recipes';
      profileUsageBar.max = limit;
      profileUsageBar.value = Math.min(count, limit);
    }

    profileMessage.textContent = editButton.disabled ? 'Account information is unavailable.' : '';
  } catch (error) {
    profileMessage.textContent = 'Could not connect to the server. Please try again.';
  }
}

// Solicita a senha apenas quando o email muda
function checkEmailChange() {
  const emailChanged = emailInput.value.trim() !== originalEmail;

  passwordConfirmation.hidden = !emailChanged;
  passwordInput.required = emailChanged;
  passwordInput.disabled = !emailChanged || saving;

  if (!emailChanged) {
    passwordInput.value = '';
    fields.current_password.error.textContent = '';
    passwordInput.removeAttribute('aria-invalid');
  }
}

// Abre o formulario de edicao
function openEditForm() {
  clearErrors();

  displayNameInput.value = originalName;
  emailInput.value = originalEmail;
  passwordInput.value = '';

  accountDetails.hidden = true;
  editForm.hidden = false;
  profileMessage.textContent = '';

  checkEmailChange();
  displayNameInput.focus();
}

// Fecha o formulario sem alterar os dados
function closeEditForm() {
  editForm.reset();
  clearErrors();

  passwordConfirmation.hidden = true;
  passwordInput.required = false;

  editForm.hidden = true;
  accountDetails.hidden = false;

  editButton.focus();
}

// Valida os dados antes de enviar ao servidor
function validateForm() {
  clearErrors();

  const displayName = displayNameInput.value.trim();
  const email = emailInput.value.trim();
  const password = passwordInput.value;
  const emailChanged = email !== originalEmail;

  const encoder = new TextEncoder();

  if (!displayName || encoder.encode(displayName).length > 100) {
    showFieldError('display_name', 'Enter a display name of at most 100 bytes.');

    displayNameInput.focus();
    return false;
  }

  const emailParts = email.split('@');
  const domain = emailParts[1] || '';

  const validEmail =
    emailParts.length === 2 &&
    emailParts[0].length > 0 &&
    domain.slice(1, -1).includes('.') &&
    !/\s/.test(email);

  if (!validEmail || encoder.encode(email).length > 254) {
    showFieldError('email', 'Please enter a valid email address.');

    emailInput.focus();
    return false;
  }

  if (emailChanged && !password.trim()) {
    showFieldError('current_password', 'Enter your current password to change your email address.');

    passwordInput.focus();
    return false;
  }

  if (emailChanged && encoder.encode(password).length > 1024) {
    showFieldError('current_password', 'Current password is too long.');

    passwordInput.focus();
    return false;
  }

  return true;
}

// Evita varios envios enquanto salva
function setSaving(value) {
  saving = value;

  saveButton.disabled = value;
  cancelButton.disabled = value;
  displayNameInput.disabled = value;
  emailInput.disabled = value;

  saveButton.textContent = value ? 'Saving...' : 'Save Changes';

  checkEmailChange();
}

// Envia as alteracoes para o backend
async function saveProfile(event) {
  event.preventDefault();

  if (saving || !validateForm()) {
    return;
  }

  const displayName = displayNameInput.value.trim();
  const email = emailInput.value.trim();
  const emailChanged = email !== originalEmail;

  const updatedProfile = {
    display_name: displayName,
    email: email,
  };

  if (emailChanged) {
    updatedProfile.current_password = passwordInput.value;
  }

  setSaving(true);
  editMessage.textContent = 'Saving your changes...';

  try {
    const response = await fetch('/api/auth/me', {
      method: 'PUT',
      credentials: 'same-origin',
      headers: {
        'Content-Type': 'application/json',
      },
      body: JSON.stringify(updatedProfile),
    });

    const data = await response.json().catch(() => ({}));

    if (response.status === 401) {
      window.location.replace('index.html');
      return;
    }

    if (!response.ok) {
      const message = data.error || 'Unable to update your account. Please try again.';

      if (data.field && fields[data.field]) {
        showFieldError(data.field, message);
      } else {
        editMessage.textContent = message;
      }

      return;
    }

    if (
      !data.user ||
      typeof data.user.display_name !== 'string' ||
      typeof data.user.email !== 'string'
    ) {
      editMessage.textContent = 'Could not confirm your changes. Reload the page to check.';
      return;
    }

    displayAccount(data.user);
    closeEditForm();

    profileMessage.textContent = 'Account information updated successfully.';
  } catch (error) {
    editMessage.textContent =
      'Could not confirm your changes. Reload the page before trying again.';
  } finally {
    setSaving(false);
  }
}

// Eventos dos botoes e do formulario
editButton.addEventListener('click', openEditForm);

cancelButton.addEventListener('click', () => {
  if (!saving) {
    closeEditForm();
  }
});

emailInput.addEventListener('input', checkEmailChange);

editForm.noValidate = true;
editForm.addEventListener('submit', saveProfile);

// Carrega o perfil quando a pagina abre
void loadProfile();
