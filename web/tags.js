const banner = document.getElementById("banner");
const tagsList = document.getElementById("tags-list");
const newTagName = document.getElementById("new-tag-name");
const newTagColor = document.getElementById("new-tag-color");
const newTagDescription = document.getElementById("new-tag-description");

function showBanner(message, kind) {
  banner.textContent = message;
  banner.className = `banner ${kind}`;
  banner.hidden = false;
}

async function api(path, options) {
  const response = await fetch(`/api${path}`, {
    headers: { "Content-Type": "application/json" },
    ...options,
  });
  const body = await response.json().catch(() => ({}));
  if (!response.ok) {
    throw new Error(body.error || `request failed (${response.status})`);
  }
  return body;
}

async function loadTags() {
  const tags = await api("/tags");
  tagsList.innerHTML = "";

  if (tags.length === 0) {
    const li = document.createElement("li");
    li.className = "empty";
    li.textContent = "No tags yet — create one above.";
    tagsList.appendChild(li);
    return;
  }

  for (const tag of tags) {
    const li = document.createElement("li");
    li.className = "tag-row";

    const colorInput = document.createElement("input");
    colorInput.type = "color";
    colorInput.value = tag.color;

    const nameInput = document.createElement("input");
    nameInput.type = "text";
    nameInput.value = tag.name;
    nameInput.className = "tag-row-name";

    const descriptionInput = document.createElement("input");
    descriptionInput.type = "text";
    descriptionInput.value = tag.description || "";
    descriptionInput.placeholder = "Description";
    descriptionInput.className = "tag-row-description";

    const preview = document.createElement("span");
    preview.className = "tag-pill";
    preview.textContent = tag.name;
    preview.style.background = tag.color;
    const syncPreview = () => {
      preview.textContent = nameInput.value || "(unnamed)";
      preview.style.background = colorInput.value;
    };
    colorInput.addEventListener("input", syncPreview);
    nameInput.addEventListener("input", syncPreview);

    const saveBtn = document.createElement("button");
    saveBtn.type = "button";
    saveBtn.className = "btn btn-secondary btn-small";
    saveBtn.textContent = "Save";
    saveBtn.addEventListener("click", async () => {
      const name = nameInput.value.trim();
      if (!name) {
        showBanner("Name is required.", "error");
        return;
      }
      try {
        await api(`/tags/${tag.id}`, {
          method: "PUT",
          body: JSON.stringify({ name, color: colorInput.value, description: descriptionInput.value.trim() }),
        });
        await loadTags();
      } catch (err) {
        showBanner(err.message, "error");
      }
    });

    const deleteBtn = document.createElement("button");
    deleteBtn.type = "button";
    deleteBtn.className = "btn btn-danger btn-small";
    deleteBtn.textContent = "Delete";
    deleteBtn.addEventListener("click", async () => {
      if (!confirm(`Delete tag "${tag.name}"? It will be removed from any items that have it.`)) {
        return;
      }
      try {
        await api(`/tags/${tag.id}`, { method: "DELETE" });
        await loadTags();
      } catch (err) {
        showBanner(err.message, "error");
      }
    });

    li.appendChild(preview);
    li.appendChild(colorInput);
    li.appendChild(nameInput);
    li.appendChild(descriptionInput);
    li.appendChild(saveBtn);
    li.appendChild(deleteBtn);
    tagsList.appendChild(li);
  }
}

document.getElementById("add-tag-button").addEventListener("click", async () => {
  const name = newTagName.value.trim();
  if (!name) {
    showBanner("Name is required.", "error");
    return;
  }
  try {
    await api("/tags", {
      method: "POST",
      body: JSON.stringify({ name, color: newTagColor.value, description: newTagDescription.value.trim() }),
    });
    newTagName.value = "";
    newTagColor.value = "#3b82f6";
    newTagDescription.value = "";
    await loadTags();
  } catch (err) {
    showBanner(err.message, "error");
  }
});

loadTags().catch((err) => showBanner(err.message, "error"));
