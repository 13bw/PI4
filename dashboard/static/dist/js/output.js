function fetchData() {
    fetch("/load")
        .then((response) => response.json())
        .then((data) => {
            updateTable(data);
        })
        .catch((error) => console.error(error));
}

function updateData() {
    fetch("/update")
        .then((response) => response.json())
        .then((data) => {
            updateTable(data);
        })
        .catch((error) => console.error(error));
}
function updateTable(data) {
    const tbody = document.querySelector("tbody");
    tbody.innerHTML = "";

    data.forEach((file) => {
        const row = document.createElement("tr");
        row.classList.add("align-middle");
        
        


        const checkboxCell = document.createElement("td");
        const checkbox = document.createElement("input");
        checkbox.type = "checkbox";
        checkboxCell.appendChild(checkbox);
        row.appendChild(checkboxCell);

        const img = document.createElement("img");
        img.src = file.path;
        img.width = 100;

        img.addEventListener("click", () => {
            img.classList.toggle("big-image");
        });
        const pathCell = document.createElement("td");
        pathCell.appendChild(img);
        row.appendChild(pathCell);

        const dateCell = document.createElement("td");
        dateCell.textContent = file.DataHora;
        row.appendChild(dateCell);

        const md5Cell = document.createElement("td");
        md5Cell.textContent = file.MD5 + ".jpg";
        row.appendChild(md5Cell);

        tbody.appendChild(row);
    });
}

window.addEventListener("load", () => {
    fetchData();
});

const update_button = document.getElementById("update");
const downloadSelected_button = document.getElementById("downloadSelected");
const selectAll_checkbox = document.getElementById("selectAll");

update_button.addEventListener("click", () => {
    updateData();
});

downloadSelected_button.addEventListener("click", () => {
    const checkboxes = document.querySelectorAll(
        'input[type="checkbox"]:checked'
    );
    const selectedImages = [];

    checkboxes.forEach((checkbox) => {
        const row = checkbox.closest("tr");
        const imgCell = row.querySelector("td:nth-child(2) img");

        if (imgCell !== null) {
            const imgSrc = imgCell.src;
            selectedImages.push(imgSrc);
        }
    });


    const filename = "selected_images.zip";

    const zip = new JSZip();
    const promises = [];

    selectedImages.forEach((imageUrl) => {
        const imageName = imageUrl.split("/").pop();
        const promise = fetch(imageUrl)
            .then((response) => response.blob())
            .then((blob) => zip.file(imageName, blob));
        promises.push(promise);
    });

    Promise.all(promises).then(() => {
        zip.generateAsync({ type: "blob" }).then((content) => {
            saveAs(content, filename);
        });
    });

    function saveAs(blob, filename) {
        const link = document.createElement("a");
        link.href = URL.createObjectURL(blob);
        link.download = filename;
        link.click();

        URL.revokeObjectURL(link.href);

        link.remove();
    }
});

selectAll_checkbox.addEventListener("change", () => {
    const checkboxes = document.querySelectorAll('input[type="checkbox"]');
    checkboxes.forEach((checkbox) => {
        checkbox.checked = selectAll_checkbox.checked;
    });
});
