import csv
import datetime
import email
import hashlib
import imaplib
import json
import os
from email.header import decode_header
from auth import username, password
from flask import Flask, render_template

csv_file_path = "data.csv"
csv_header = ["path", "DataHora", "MD5"]
csv_exists = os.path.exists(csv_file_path)
with open(csv_file_path, "a", newline="") as csvfile:
    writer = csv.writer(csvfile)
    if not csv_exists:
        writer.writerow(csv_header)


save_path = "static/attachments"
if not os.path.exists(save_path):
    os.mkdir(save_path)


def update_images():
    with open(csv_file_path, "a", newline="") as csvfile:
        writer = csv.writer(csvfile)
        if not csv_exists:
            writer.writerow(csv_header)

    mail = imaplib.IMAP4_SSL("imap.gmail.com")
    mail.login(username, password)

    mail.select("inbox")

    result, data = mail.search(None, "ALL")
    ids = data[0]

    for id in ids.split():
        result, data = mail.fetch(id, "(RFC822)")
        raw_email = data[0][1]

        msg = email.message_from_bytes(raw_email)

        subject = decode_header(msg["Subject"])[0][0]
        from_ = decode_header(msg.get("From"))[0][0]
        to_ = decode_header(msg.get("To"))[0][0]

        date_time_str = msg.get("Date")

        received_time = datetime.datetime.strptime(date_time_str, "%a, %d %b %Y %H:%M:%S %z")

        print("From:", from_)
        print("To:", to_)
        print("Subject:", subject)

        print("Received Date:", received_time.strftime("%Y-%m-%d %H:%M:%S"))

        if msg.is_multipart():
            for part in msg.walk():
                try:
                    content_disposition = str(part.get("Content-Disposition"))

                    if "attachment" in content_disposition:
                        filename = part.get_filename()
                        if filename:
                            filepath = os.path.join(save_path, filename)
                            with open(filepath, "wb") as f:
                                f.write(part.get_payload(decode=True))
                            print(f"Attachment saved: {filepath}")

                            md5_hash = hashlib.md5()
                            with open(filepath, "rb") as f:
                                for chunk in iter(lambda: f.read(4096), b""):
                                    md5_hash.update(chunk)
                            md5_digest = md5_hash.hexdigest()

                            new_filepath = os.path.join(
                                save_path,
                                f"{md5_digest}{os.path.splitext(filename)[1]}",
                            )
                            os.rename(filepath, new_filepath)
                            filepath = new_filepath

                            with open(csv_file_path, "a", newline="") as csvfile:
                                writer = csv.writer(csvfile)
                                writer.writerow([filepath, received_time, md5_digest])

                except Exception as e:
                    print("Error while saving attachment", e)

        print("=" * 50)

    mail.close()
    mail.logout()


app = Flask(__name__)


@app.route("/")
@app.route("/index")
def index():
    return render_template("index.html")


@app.route("/update")
def update():
    update_images()
    with open(csv_file_path, "r") as f:
        reader = csv.reader(f)
        next(reader)
        data = [{"path": row[0], "DataHora": row[1], "MD5": row[2]} for row in reader]

    return app.response_class(
        response=json.dumps(data), status=200, mimetype="application/json"
    )


@app.route("/load")
def load():
    with open(csv_file_path, "r") as f:
        reader = csv.reader(f)
        next(reader)
        data = [{"path": row[0], "DataHora": row[1], "MD5": row[2]} for row in reader]

    return app.response_class(
        response=json.dumps(data), status=200, mimetype="application/json"
    )


if __name__ == "__main__":
    app.run(debug=True)
