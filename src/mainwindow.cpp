#include "mainwindow.h"      // header for MainWindow class

// Qt GUI and utility includes
#include <QFileDialog>       // file open/save dialog
#include <QVBoxLayout>       // vertical layout manager
#include <QHBoxLayout>       // horizontal layout manager
#include <QFile>             // file I/O (read/write files)
#include <QJsonDocument>     // handle JSON documents
#include <QJsonObject>       // JSON objects
#include <QMessageBox>       // popup message dialogs
#include <QDir>              // directory handling
#include <QFileInfo>         // file information (name, size, path, etc.)
#include <QTextStream>       // read/write text to files

// Crypto++ includes
#include <cryptopp/sha.h>    // SHA hashing (SHA-1, SHA-256, etc.)
#include <cryptopp/hmac.h>   // HMAC (keyed hash for integrity/auth)
#include <cryptopp/aes.h>    // AES block cipher
#include <cryptopp/modes.h>  // encryption modes (CBC, CFB, etc.)
#include <cryptopp/filters.h>// stream filters (StringSource, StreamTransformationFilter, etc.)
#include <cryptopp/osrng.h>  // secure random number generator
#include <cryptopp/hex.h>    // hex encoding/decoding

using namespace CryptoPP;


// ---------------- Helper functions ------------------

// Performs a constant-time comparison of two strings to prevent timing attacks.
// Returns true if the strings are equal, false otherwise.
static bool constantTimeEqual(const std::string &a, const std::string &b) {
    if (a.size() != b.size()) return false;
    unsigned char diff = 0;
    for (size_t i = 0; i < a.size(); ++i) 
        diff |= ((unsigned char)a[i] ^ (unsigned char)b[i]); // accumulate differences
    return diff == 0;
}

// Computes the HMAC-SHA256 of the given data using the provided HMAC key.
// Returns the raw binary MAC (32 bytes).
static std::string computeHmacSha256(const QByteArray &data, const SecByteBlock &hmacKey) {
    std::string mac;
    HMAC<SHA256> h((const byte*)hmacKey.BytePtr(), hmacKey.size());  // initialize HMAC
    StringSource ss((const byte*)data.constData(), data.size(), true,
                    new HashFilter(h, new StringSink(mac)));          // compute HMAC
    return mac; // raw binary MAC
}


// ---------- MainWindow implementation ----------
MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    QWidget* central = new QWidget;
    setCentralWidget(central);

    uploadBtn = new QPushButton("Upload");
    processBtn = new QPushButton("Process");
    downloadBtn = new QPushButton("Download");
    genKeyBtn = new QPushButton("Generate Key");

    opCombo = new QComboBox;
    opCombo->addItem("Generate Symmetric Key");
    opCombo->addItem("AES Encrypt (file)");
    opCombo->addItem("AES Decrypt (file)");
    opCombo->addItem("SHA-256 Digest (file)");
    opCombo->addItem("HMAC-SHA256 (file)");
    // opCombo->addItem("Verify HMAC (file with appended MAC)");

    keyHexEdit = new QLineEdit;
    keyHexEdit->setPlaceholderText("Symmetric key (hex) — or click Generate Key");

    hmacKeyEdit = new QLineEdit;
    hmacKeyEdit->setPlaceholderText("HMAC key (hex) optional");

    progressBar = new QProgressBar;
    progressBar->setRange(0, 100);
    progressBar->setValue(0);

    statusLabel = new QLabel("Idle");
    outputText = new QTextEdit;
    outputText->setReadOnly(true);
    outputText->setFixedHeight(120);

    QHBoxLayout* topRow = new QHBoxLayout;
    topRow->addWidget(uploadBtn);
    topRow->addWidget(processBtn);
    topRow->addWidget(downloadBtn);
    topRow->addWidget(genKeyBtn);

    QVBoxLayout* layout = new QVBoxLayout;
    layout->addWidget(opCombo);
    layout->addWidget(keyHexEdit);
    layout->addWidget(hmacKeyEdit);
    layout->addLayout(topRow);
    layout->addWidget(progressBar);
    layout->addWidget(statusLabel);
    layout->addWidget(outputText);

    central->setLayout(layout);

    connect(uploadBtn, &QPushButton::clicked, this, &MainWindow::onUpload);
    connect(processBtn, &QPushButton::clicked, this, &MainWindow::onProcess);
    connect(downloadBtn, &QPushButton::clicked, this, &MainWindow::onDownload);
    connect(genKeyBtn, &QPushButton::clicked, this, &MainWindow::onGenerateKey);

    loadConfig();
    setWindowTitle("Crypto S/W App1");
    resize(720, 480);
}


// Updates the status label in the GUI with the given message.
void MainWindow::setStatus(const QString& s) {
    statusLabel->setText(s);
}


// Loads cryptographic configuration from "config.json".
// If the file doesn't exist or is invalid, uses default values.
void MainWindow::loadConfig() {
    QFile f("config.json");
    if (!f.open(QFile::ReadOnly)) {                // try to open config file
        setStatus("Could not open config.json — using defaults");
        return;                                    // use defaults if file missing
    }

    QByteArray data = f.readAll();                // read entire file
    QJsonDocument doc = QJsonDocument::fromJson(data);  // parse JSON
    if (!doc.isObject()) {                         // check if valid JSON object
        setStatus("config.json invalid — using defaults");
        return;                                    // use defaults if invalid
    }

    QJsonObject obj = doc.object();
    // read config values, provide defaults if missing
    aesKeyBytes   = obj.value("aes_key_bytes").toInt(32);
    aesIvBytes    = obj.value("aes_iv_bytes").toInt(16);
    hmacKeyBytes  = obj.value("hmac_key_bytes").toInt(32);
}


// Reads the entire contents of the specified file into a QByteArray.
// Returns true if successful, false if the file could not be opened.
bool MainWindow::readFileToByteArray(const QString& path, QByteArray& out) {
    QFile f(path);
    if (!f.open(QFile::ReadOnly)) return false;  // try to open file for reading
    out = f.readAll();                            // read all data into QByteArray
    return true;                                  // success
}


// Writes a QByteArray to the specified file path.
// Returns true if the entire data was successfully written, false otherwise.
bool MainWindow::writeByteArrayToFile(const QString& path, const QByteArray& data) {
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) return false;  // open file for writing
    qint64 written = f.write(data);  // write all bytes
    f.close();                        // close file
    return (written == data.size());  // check if full data was written
}


// Opens a file dialog for the user to select a file, 
// stores the path, resets progress/output, and updates status
void MainWindow::onUpload() {
    QString file = QFileDialog::getOpenFileName(this, "Open file");
    if (file.isEmpty()) return;  // user canceled
    inputFilePath = file;
    setStatus(QString("Selected: %1").arg(file));
    progressBar->setValue(0);    // reset progress bar
    outputText->clear();          // clear previous output
    processedData.clear();        // clear any previous processed data
    lastOutputIsText = false;     // reset output type
    lastTextOutput.clear();       // clear last text output
    lastAction = LastAction::None; // reset last action
}


// Generates a new random symmetric AES key and HMAC key, 
// displays them in the GUI in hex format, and updates internal state
void MainWindow::onGenerateKey() {
    AutoSeededRandomPool rng;

    // Generate symmetric AES key
    SecByteBlock symKey(aesKeyBytes);
    rng.GenerateBlock(symKey, symKey.size());
    std::string symHex;
    HexEncoder hexEnc1(new StringSink(symHex));
    hexEnc1.Put(symKey, symKey.size());
    hexEnc1.MessageEnd();

    // Generate HMAC key
    SecByteBlock hmacKey(hmacKeyBytes);
    rng.GenerateBlock(hmacKey, hmacKey.size());
    std::string hmacHex;
    HexEncoder hexEnc2(new StringSink(hmacHex));
    hexEnc2.Put(hmacKey, hmacKey.size());
    hexEnc2.MessageEnd();

    // Update GUI fields
    keyHexEdit->setText(QString::fromStdString(symHex));
    hmacKeyEdit->setText(QString::fromStdString(hmacHex));

    // Update internal tracking state
    lastGeneratedSymKeyHex = QString::fromStdString(symHex);
    lastGeneratedHmacKeyHex = QString::fromStdString(hmacHex);
    lastAction = LastAction::GeneratedKey;
    processedData.clear();
    lastOutputIsText = false;
    lastTextOutput.clear();

    // Update status and output messages
    setStatus("Generated symmetric key and HMAC key (shown in hex)");
    outputText->setPlainText("Symmetric and HMAC keys generated. Click Download to save the key pair.");
}


// Saves the last generated key pair or processed output to a file,
// using an appropriate suggested filename/extension based on the operation.
void MainWindow::onDownload() {
    // Case 1: last action was key generation
    if (lastAction == LastAction::GeneratedKey) {
        // Suggest filename based on input file or default "keypair"
        QString base = QFileInfo(inputFilePath).completeBaseName();
        if (base.isEmpty()) base = "keypair";
        QString suggested = base + ".keypair.hex";
        QString file = QFileDialog::getSaveFileName(this, "Save key pair", suggested, "Key pair (*.keypair.hex);;All Files (*)");
        if (file.isEmpty()) return; // user canceled

        // Write keys to file
        QFile f(file);
        if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            setStatus("Failed to save key pair");
            return;
        }
        QTextStream out(&f);
        out << "symmetric_key_hex:" << (lastGeneratedSymKeyHex.isEmpty() ? keyHexEdit->text() : lastGeneratedSymKeyHex) << "\n";
        out << "hmac_key_hex:" << (lastGeneratedHmacKeyHex.isEmpty() ? hmacKeyEdit->text() : lastGeneratedHmacKeyHex) << "\n";
        f.close();
        setStatus(QString("Saved key pair %1").arg(file));
        QMessageBox::information(this, "Saved", "Key pair saved.");
        return;
    }

    // Case 2: No processed data to save
    if (processedData.isEmpty() && outputText->toPlainText().isEmpty()) {
        QMessageBox::information(this, "Nothing to save", "No processed data to save. Run Process first.");
        return;
    }

    // Determine suggested filename and extension based on current operation
    QString baseName = QFileInfo(inputFilePath).completeBaseName();
    if (baseName.isEmpty()) baseName = "output";
    QString op = opCombo->currentText();
    QString suggestedExt;
    if (op.contains("AES Encrypt", Qt::CaseInsensitive)) suggestedExt = ".aescbc";
    else if (op.contains("AES Decrypt", Qt::CaseInsensitive)) suggestedExt = (lastOutputIsText ? ".txt" : ".bin");
    else if (op.contains("SHA-256", Qt::CaseInsensitive)) suggestedExt = ".sha256";
    else if (op.contains("HMAC-SHA256", Qt::CaseInsensitive)) suggestedExt = ".hmac";
    else suggestedExt = (lastOutputIsText ? ".txt" : ".bin");

    QString defaultName = baseName;
    if (!defaultName.endsWith(suggestedExt, Qt::CaseInsensitive)) defaultName += suggestedExt;

    QString file = QFileDialog::getSaveFileName(this, "Save output", defaultName, "All Files (*)");
    if (file.isEmpty()) return; // user canceled

    // Ensure text outputs have .txt extension if missing
    if (lastOutputIsText && QFileInfo(file).suffix().isEmpty()) file += ".txt";

    // Save processed binary or text data
    if (!processedData.isEmpty()) {
        if (lastOutputIsText) { // text output
            QFile f(file);
            if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
                setStatus("Failed to save text output");
                return;
            }
            QByteArray outBytes = lastTextOutput.toUtf8();
            f.write(outBytes);
            f.close();
            setStatus(QString("Saved text %1").arg(file));
            QMessageBox::information(this, "Saved", "Text output saved.");
            return;
        }
        // binary output
        if (!writeByteArrayToFile(file, processedData)) {
            setStatus("Failed to save output file");
            return;
        }
        setStatus(QString("Saved %1").arg(file));
        QMessageBox::information(this, "Saved", "Output file saved.");
    } else { // fallback: write outputText content
        QFile f(file);
        if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            setStatus("Failed to save text output");
            return;
        }
        f.write(outputText->toPlainText().toUtf8());
        f.close();
        setStatus(QString("Saved text %1").arg(file));
        QMessageBox::information(this, "Saved", "Text output saved.");
    }
}

// Processes the input file or text according to the selected operation in the GUI.
// Supports AES encryption/decryption, SHA-256 hashing, and HMAC-SHA256.
// Updates the progress bar, output text, and internal state with the processed data.
void MainWindow::onProcess() {
    if (opCombo->currentText() == "Generate Symmetric Key") {
        onGenerateKey();
        return;
    }

    // For other operations, read input file first
    if (inputFilePath.isEmpty()) {
        QMessageBox::warning(this, "No file", "Please upload a file first.");
        return;
    }

    QByteArray inputData;
    if (!readFileToByteArray(inputFilePath, inputData)) {
        setStatus("Failed to read input file");
        return;
    }
    progressBar->setValue(10);

    try {
        QString op = opCombo->currentText();

        if (op == "AES Encrypt (file)") {
            // ensure symmetric key present; if not, generate one and show it
            if (keyHexEdit->text().isEmpty()) {
                onGenerateKey(); // populates keyHexEdit (and hmacKeyEdit too)
            }

            // decode symmetric key from hex
            std::string keyHex = keyHexEdit->text().toStdString();
            SecByteBlock key(aesKeyBytes);
            StringSource ss1(keyHex, true, new HexDecoder(new ArraySink(key, key.size())));

            // generate IV
            AutoSeededRandomPool rng;
            SecByteBlock iv(aesIvBytes);
            rng.GenerateBlock(iv, iv.size());

            // encrypt inputData -> ciphertext (PKCS padding)
            std::string ciphertext;
            CBC_Mode<AES>::Encryption e;
            e.SetKeyWithIV(key, key.size(), iv, iv.size());
            StringSource ss2((const byte*)inputData.constData(), inputData.size(), true,
                new StreamTransformationFilter(e, new StringSink(ciphertext), StreamTransformationFilter::PKCS_PADDING)
            );

            // Build processedData = IV || ciphertext  (NO HMAC appended)
            processedData.clear();
            processedData.append(reinterpret_cast<const char*>(iv.BytePtr()), iv.size());
            processedData.append(ciphertext.data(), (int)ciphertext.size());

            outputText->setPlainText(QString("Encryption successful. Ciphertext size (IV + ciphertext): %1 bytes").arg(processedData.size()));
            setStatus("Encryption done (no HMAC)");
            progressBar->setValue(100);
            lastAction = LastAction::ProcessedData;
            lastOutputIsText = false;
        }

        else if (op == "AES Decrypt (file)") {
            // Expect input: IV || ciphertext  (no HMAC)
            if (inputData.size() < aesIvBytes) {
                setStatus("Input too small to contain IV");
                return;
            }

            // extract IV and ciphertext
            QByteArray ivBytes = inputData.left(aesIvBytes);
            QByteArray cipherOnly = inputData.mid(aesIvBytes);

            // require symmetric key
            if (keyHexEdit->text().isEmpty()) {
                QMessageBox::warning(this, "Key required", "Please provide symmetric key (hex) or click Generate Key.");
                return;
            }
            std::string keyHex = keyHexEdit->text().toStdString();
            SecByteBlock key(aesKeyBytes);
            StringSource ssKey(keyHex, true, new HexDecoder(new ArraySink(key, key.size())));

            // perform decryption (Crypto++ writes plaintext to std::string recovered)
            std::string recovered;
            CBC_Mode<AES>::Decryption d;
            d.SetKeyWithIV(key, key.size(), (const byte*)ivBytes.constData(), ivBytes.size());
            StringSource ss3((const byte*)cipherOnly.constData(), cipherOnly.size(), true,
                new StreamTransformationFilter(d, new StringSink(recovered), StreamTransformationFilter::PKCS_PADDING)
            );

            // copy recovered bytes (avoid fromRawData)
            processedData = QByteArray(recovered.data(), static_cast<int>(recovered.size()));

            // Text detection & preview (UTF-8 or UTF-16-LE)
            lastOutputIsText = false;
            lastTextOutput.clear();
            if (!processedData.isEmpty()) {
                QString maybeUtf8 = QString::fromUtf8(processedData);
                if (maybeUtf8.toUtf8() == processedData) {
                    lastOutputIsText = true;
                    lastTextOutput = maybeUtf8;
                    outputText->setPlainText(lastTextOutput.left(10000));
                } else {
                    // check UTF-16-LE
                    bool looksUtf16Le = false;
                    if (processedData.size() >= 2) {
                        if ((uint8_t)processedData[0] == 0xFF && (uint8_t)processedData[1] == 0xFE)
                            looksUtf16Le = true;
                        else {
                            int zeros = 0;
                            int limit = qMin(processedData.size()-1, 200);
                            for (int i = 1; i < limit; i += 2) if (processedData[i] == '\0') ++zeros;
                            if (zeros > 3) looksUtf16Le = true;
                        }
                    }
                    if (looksUtf16Le && (processedData.size() % 2 == 0)) {
                        const ushort* u16 = reinterpret_cast<const ushort*>(processedData.constData());
                        int u16len = processedData.size() / 2;
                        lastTextOutput = QString::fromUtf16(u16, u16len);
                        lastOutputIsText = true;
                        outputText->setPlainText(lastTextOutput.left(10000));
                    } else {
                        outputText->setPlainText(QString("Decryption successful. Plaintext size: %1 bytes").arg(processedData.size()));
                    }
                }
            } else {
                outputText->setPlainText("Decryption produced empty output");
            }

            setStatus("Decryption done");
            progressBar->setValue(100);
            lastAction = LastAction::ProcessedData;
        }

        else if (op == "SHA-256 Digest (file)") {
            SHA256 hash;
            std::string digest;
            StringSource ss((const byte*)inputData.constData(), inputData.size(), true,
                new HashFilter(hash, new HexEncoder(new StringSink(digest), false))
            );
            outputText->setPlainText(QString::fromStdString(digest));
            processedData.clear();
            setStatus("SHA-256 generated");
            progressBar->setValue(100);
            lastAction = LastAction::ShaOrHmacText;
            lastOutputIsText = true;
            lastTextOutput = outputText->toPlainText();
        }
        else if (op == "HMAC-SHA256 (file)") {
            SecByteBlock hmacKey(hmacKeyBytes);
            bool hmacWasAutoGenerated = false;
            if (!hmacKeyEdit->text().isEmpty()) {
                std::string hkHex = hmacKeyEdit->text().toStdString();
                StringSource ss(hkHex, true, new HexDecoder(new ArraySink(hmacKey, hmacKey.size())));
            } else if (!keyHexEdit->text().isEmpty()) {
                std::string hkHex = keyHexEdit->text().toStdString();
                StringSource ss(hkHex, true, new HexDecoder(new ArraySink(hmacKey, hmacKey.size())));
            } else {
                AutoSeededRandomPool rng;
                rng.GenerateBlock(hmacKey, hmacKey.size());
                std::string hexOut;
                HexEncoder encoder(new StringSink(hexOut));
                encoder.Put(hmacKey, hmacKey.size());
                encoder.MessageEnd();
                hmacKeyEdit->setText(QString::fromStdString(hexOut));
                lastGeneratedHmacKeyHex = QString::fromStdString(hexOut);
                hmacWasAutoGenerated = true;
            }

            // Create HMAC object
            HMAC<SHA256> h((const byte*)hmacKey.BytePtr(), hmacKey.size());

            // 1) compute raw MAC bytes
            SecByteBlock mac(h.DigestSize());
            h.CalculateDigest(mac, (const byte*)inputData.constData(), inputData.size());

            // 2) hex-encode MAC for display/ textual append
            std::string macHex;
            StringSource ss2(mac, mac.size(), true,
                new HexEncoder(new StringSink(macHex), false)
            );

            // --- Append raw MAC bytes to processedData (binary) so you can save the original+MAC as a file ---
            processedData = inputData; // copy original bytes
            processedData.append(reinterpret_cast<const char*>(mac.BytePtr()), mac.size());

            // --- Create a human-readable text: original text + MAC(hex) and show it in UI ---
            std::string origStr((const char*)inputData.constData(), inputData.size());
            std::string combinedText = origStr + macHex;
            outputText->setPlainText(QString::fromStdString(combinedText));

            // Update UI & state
            setStatus("HMAC-SHA256 generated and appended");
            progressBar->setValue(100);
            lastAction = LastAction::ShaOrHmacText;
            lastOutputIsText = true;
            lastTextOutput = outputText->toPlainText();
        }
        else {
            setStatus("Operation not implemented yet");
            return;
        }
    } catch (const Exception& e) {
        setStatus(QString("Crypto++ error: %1").arg(QString::fromStdString(e.what())));
    } catch (const std::exception& e) {
        setStatus(QString("Error: %1").arg(e.what()));
    } catch (...) {
        setStatus("Unknown error during processing");
    }
}
