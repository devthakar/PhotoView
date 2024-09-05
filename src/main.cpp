#include <wx/wx.h>
#include <wx/sizer.h>
#include <wx/timer.h>
#include <wx/slider.h>
#include <wx/filedlg.h>
#include <wx/grid.h>
#include <fstream>
#include <algorithm> 

const wxString DATA_FILE = "album_data.txt"; 

class MyApp : public wxApp
{
public:
    virtual bool OnInit();
};

class MyFrame : public wxFrame
{
public:
    MyFrame(const wxString& title);

    void OnCreateAlbum(wxCommandEvent& event);
    void OnAlbumClick(wxMouseEvent& event);
    void ShowAlbumPage(const wxString& title, std::vector<wxBitmap>& photos, std::vector<wxString>& photoPaths);
    void OnAddPhoto(wxCommandEvent& event);
    void OnRemovePhoto(wxCommandEvent& event);
    void OnNext(wxCommandEvent& event);
    void OnBack(wxCommandEvent& event);
    void OnPhotoClick(wxMouseEvent& event); 
    void OnPhotoHover(wxMouseEvent& event); 
    void OnTimer(wxTimerEvent& event);      
    void ResetPhotoPosition(wxMouseEvent& event);  

    void SaveAlbumData(); 
    void LoadAlbumData(); 

private:
    wxBoxSizer* mainSizer;
    wxBoxSizer* buttonSizer;
    wxGridSizer* albumGridSizer;
    wxGridSizer* photoSizer;

    std::vector<wxStaticBitmap*> albumWidgets;
    std::vector<std::vector<wxBitmap> > albums;  
    std::vector<std::vector<wxString> > albumPaths; 
    std::vector<wxString> albumTitles; 
    std::vector<wxBitmap> currentAlbum;  
    std::vector<wxString> currentPaths;  
    wxString currentAlbumTitle;  
    std::vector<wxStaticBitmap*> photoWidgets;
    int currentStartIndex;

    wxTimer* hoverTimer;   
    wxStaticBitmap* hoverPhoto;  
    int originalY;          

    void UpdateAlbumDisplay();
    void UpdatePhotoDisplay();

    wxDECLARE_EVENT_TABLE();
};

class AlbumFrame : public wxFrame
{
public:
    AlbumFrame(const wxString& title, std::vector<wxBitmap>& photos, std::vector<wxString>& photoPaths, MyFrame* parent);

    void OnAddPhoto(wxCommandEvent& event);
    void OnPhotoClick(wxMouseEvent& event);
    void OnBackToMain(wxCommandEvent& event);

private:
    wxBoxSizer* mainSizer;
    wxGridSizer* photoSizer;
    std::vector<wxBitmap>& albumPhotos;
    std::vector<wxString>& albumPhotoPaths;
    std::vector<wxStaticBitmap*> photoWidgets;
    MyFrame* parentFrame;  

    void UpdatePhotoDisplay();

    wxDECLARE_EVENT_TABLE();
};

class CreateAlbumDialog : public wxDialog
{
public:
    CreateAlbumDialog(wxWindow* parent);

    wxString GetAlbumTitle() const { return albumTitle->GetValue(); }
    wxImage GetAlbumCover() const { return coverPhoto; }

private:
    wxTextCtrl* albumTitle;
    wxButton* uploadButton;
    wxStaticBitmap* photoPreview;  
    wxImage coverPhoto;

    void OnUploadPhoto(wxCommandEvent& event);
};

class PhotoEditorFrame : public wxFrame
{
public:
    PhotoEditorFrame(wxBitmap bitmap);

private:
    wxStaticBitmap* photoDisplay;
    wxImage originalImage;
    wxSlider* brightnessSlider;
    wxSlider* saturationSlider;
    wxSlider* contrastSlider;  

    void OnBrightnessChange(wxCommandEvent& event);
    void OnSaturationChange(wxCommandEvent& event);
    void OnContrastChange(wxCommandEvent& event);  

    wxImage AdjustBrightness(wxImage image, int value);
    wxImage AdjustSaturation(wxImage image, int value);
    wxImage AdjustContrast(wxImage image, int value);  

    wxDECLARE_EVENT_TABLE();
};

enum
{
    ID_CreateAlbum = 1,
    ID_AddPhoto = 2,
    ID_RemovePhoto = 3,
    ID_Next = 4,
    ID_Back = 5,
    ID_Timer = 6,
    ID_BrightnessSlider = 7,
    ID_SaturationSlider = 8,
    ID_ContrastSlider = 9,  
    ID_UploadPhoto = 10,
    ID_BackToMain = 11  
};

wxBEGIN_EVENT_TABLE(MyFrame, wxFrame)
    EVT_BUTTON(ID_CreateAlbum, MyFrame::OnCreateAlbum)
    EVT_BUTTON(ID_AddPhoto, MyFrame::OnAddPhoto)
    EVT_BUTTON(ID_RemovePhoto, MyFrame::OnRemovePhoto)
    EVT_BUTTON(ID_Next, MyFrame::OnNext)
    EVT_BUTTON(ID_Back, MyFrame::OnBack)
    EVT_TIMER(ID_Timer, MyFrame::OnTimer)
wxEND_EVENT_TABLE()

wxBEGIN_EVENT_TABLE(AlbumFrame, wxFrame)
    EVT_BUTTON(ID_AddPhoto, AlbumFrame::OnAddPhoto)
    EVT_BUTTON(ID_BackToMain, AlbumFrame::OnBackToMain)  
wxEND_EVENT_TABLE()

wxBEGIN_EVENT_TABLE(PhotoEditorFrame, wxFrame)
    EVT_SLIDER(ID_BrightnessSlider, PhotoEditorFrame::OnBrightnessChange)
    EVT_SLIDER(ID_SaturationSlider, PhotoEditorFrame::OnSaturationChange)
    EVT_SLIDER(ID_ContrastSlider, PhotoEditorFrame::OnContrastChange)  
wxEND_EVENT_TABLE()

wxIMPLEMENT_APP(MyApp);

bool MyApp::OnInit()
{
    wxInitAllImageHandlers();  

    MyFrame *frame = new MyFrame("Photo Albums");
    frame->Show(true);
    return true;
}

MyFrame::MyFrame(const wxString& title)
    : wxFrame(NULL, wxID_ANY, title, wxDefaultPosition, wxSize(800, 600)),
      currentStartIndex(0), hoverPhoto(NULL), originalY(0)
{
    
    mainSizer = new wxBoxSizer(wxVERTICAL);

    
    wxButton* createAlbumButton = new wxButton(this, ID_CreateAlbum, "Create New Album");
    mainSizer->Add(createAlbumButton, 0, wxALL | wxALIGN_CENTER_HORIZONTAL, 10);

    
    albumGridSizer = new wxGridSizer(0, 3, 10, 10);  
    mainSizer->Add(albumGridSizer, 1, wxEXPAND | wxALL, 10);

    photoSizer = new wxGridSizer(0, 3, 10, 10);  

    mainSizer->Add(photoSizer, 1, wxEXPAND | wxALL, 10);
    SetSizer(mainSizer);
    Layout();
    hoverTimer = new wxTimer(this, ID_Timer);
    LoadAlbumData();
}

void MyFrame::OnCreateAlbum(wxCommandEvent& event)
{
    CreateAlbumDialog createAlbumDialog(this);
    if (createAlbumDialog.ShowModal() == wxID_OK)
    {
        wxString title = createAlbumDialog.GetAlbumTitle();
        wxImage cover = createAlbumDialog.GetAlbumCover();

        if (!cover.IsOk())
        {
            wxMessageBox("Failed to load album cover.", "Error", wxOK | wxICON_ERROR);
            return;
        }


        std::vector<wxBitmap> newAlbum;
        std::vector<wxString> newPaths;
        wxBitmap coverBitmap(cover);
        newAlbum.push_back(coverBitmap);
        newPaths.push_back(wxString()); 
        albums.push_back(newAlbum);
        albumPaths.push_back(newPaths);
        albumTitles.push_back(title);

        UpdateAlbumDisplay();
        SaveAlbumData(); 
    }
}

void MyFrame::UpdateAlbumDisplay()
{

    for (size_t i = 0; i < albumWidgets.size(); ++i)
    {
        albumGridSizer->Detach(albumWidgets[i]);
        albumWidgets[i]->Destroy();
    }
    albumWidgets.clear();

    for (size_t i = 0; i < albums.size(); ++i)
    {
        wxBoxSizer* albumBoxSizer = new wxBoxSizer(wxVERTICAL);

        wxStaticText* albumTitle = new wxStaticText(this, wxID_ANY, albumTitles[i], wxDefaultPosition, wxSize(100, 20), wxALIGN_CENTER);
        albumBoxSizer->Add(albumTitle, 0, wxALIGN_CENTER_HORIZONTAL | wxBOTTOM, 5);
        
        wxStaticBitmap* albumCoverWidget = new wxStaticBitmap(this, wxID_ANY, albums[i][0], wxDefaultPosition, wxSize(100, 100));
        albumCoverWidget->Bind(wxEVT_LEFT_DOWN, &MyFrame::OnAlbumClick, this);
        albumBoxSizer->Add(albumCoverWidget, 0, wxALIGN_CENTER_HORIZONTAL);

        albumGridSizer->Add(albumBoxSizer, 0, wxEXPAND);
        albumWidgets.push_back(albumCoverWidget);
    }

    Layout();
}

void MyFrame::OnAlbumClick(wxMouseEvent& event)
{
    wxStaticBitmap* clickedAlbum = static_cast<wxStaticBitmap*>(event.GetEventObject());
    size_t index = std::distance(albumWidgets.begin(), std::find(albumWidgets.begin(), albumWidgets.end(), clickedAlbum));

    if (index < albums.size())
    {
        currentAlbum = albums[index];
        currentPaths = albumPaths[index];
        currentAlbumTitle = albumTitles[index];
        ShowAlbumPage(currentAlbumTitle, currentAlbum, currentPaths);
    }
}

void MyFrame::ShowAlbumPage(const wxString& title, std::vector<wxBitmap>& photos, std::vector<wxString>& photoPaths)
{
    AlbumFrame* albumFrame = new AlbumFrame(title, photos, photoPaths, this);
    albumFrame->Show();
    this->Hide();  
}

void MyFrame::OnAddPhoto(wxCommandEvent& event)
{
    wxFileDialog openFileDialog(this, _("Open Image file"), "", "",
        "Image files (*.png;*.jpg;*.jpeg)|*.png;*.jpg;*.jpeg", wxFD_OPEN | wxFD_FILE_MUST_EXIST);

    if (openFileDialog.ShowModal() == wxID_CANCEL)
        return; 

    wxString path = openFileDialog.GetPath();

    wxBitmap photo;
    photo.LoadFile(path, wxBITMAP_TYPE_ANY);

    if (photo.IsOk())
    {
        currentAlbum.push_back(photo);
        currentPaths.push_back(path);  

        wxMessageBox("Photo uploaded successfully!\nFile path: " + path, "Upload Confirmation", wxOK | wxICON_INFORMATION);

        UpdatePhotoDisplay();
        SaveAlbumData(); 
    }
    else
    {
        wxMessageBox("Failed to load the selected photo.", "Error", wxOK | wxICON_ERROR);
    }
}

void MyFrame::OnRemovePhoto(wxCommandEvent& event)
{
    if (!currentAlbum.empty()) {
        currentAlbum.pop_back();
        currentPaths.pop_back();
        UpdatePhotoDisplay();
        SaveAlbumData(); 
    }
}

void MyFrame::OnNext(wxCommandEvent& event)
{
    if (currentStartIndex + 3 < currentAlbum.size()) {
        currentStartIndex += 3;
        UpdatePhotoDisplay();
    }
}

void MyFrame::OnBack(wxCommandEvent& event)
{
    if (currentStartIndex >= 3) {
        currentStartIndex -= 3;
        UpdatePhotoDisplay();
    }
}

void MyFrame::UpdatePhotoDisplay()
{
    for (size_t i = 0; i < photoWidgets.size(); ++i) {
        photoSizer->Detach(photoWidgets[i]);
        photoWidgets[i]->Destroy();
    }
    photoWidgets.clear();

    for (int i = 0; i < 3; ++i) {
        if (currentStartIndex + i < currentAlbum.size()) {
            wxStaticBitmap* photo = new wxStaticBitmap(this, wxID_ANY, currentAlbum[currentStartIndex + i]);

            photo->Bind(wxEVT_ENTER_WINDOW, &MyFrame::OnPhotoHover, this);
            photo->Bind(wxEVT_LEAVE_WINDOW, &MyFrame::ResetPhotoPosition, this);
            photo->Bind(wxEVT_LEFT_DOWN, &MyFrame::OnPhotoClick, this);

            photoSizer->Add(photo, 0, wxEXPAND);
            photoWidgets.push_back(photo);
        }
    }

    Layout();
}

void MyFrame::OnPhotoHover(wxMouseEvent& event)
{
    wxStaticBitmap* photo = static_cast<wxStaticBitmap*>(event.GetEventObject());
    hoverPhoto = photo;
    originalY = photo->GetPosition().y;  
    hoverTimer->Start(50);  
}

void MyFrame::OnTimer(wxTimerEvent& event)
{
    if (hoverPhoto) {
        static int offset = 0;
        offset = (offset + 1) % 8;
        int hopAmount = (offset < 4) ? -2 : 2;  
        hoverPhoto->Move(hoverPhoto->GetPosition() + wxPoint(0, hopAmount));
    }
}

void MyFrame::ResetPhotoPosition(wxMouseEvent& event)
{
    if (hoverPhoto) {
        hoverTimer->Stop();  
        hoverPhoto->Move(wxPoint(hoverPhoto->GetPosition().x, originalY));  
        hoverPhoto = NULL;
    }
}

void MyFrame::OnPhotoClick(wxMouseEvent& event)
{
    wxStaticBitmap* photo = static_cast<wxStaticBitmap*>(event.GetEventObject());
    wxBitmap bitmap = photo->GetBitmap();

    PhotoEditorFrame* editorFrame = new PhotoEditorFrame(bitmap);
    editorFrame->Show();
}


AlbumFrame::AlbumFrame(const wxString& title, std::vector<wxBitmap>& photos, std::vector<wxString>& photoPaths, MyFrame* parent)
    : wxFrame(NULL, wxID_ANY, title, wxDefaultPosition, wxSize(800, 600)), albumPhotos(photos), albumPhotoPaths(photoPaths), parentFrame(parent)
{
    mainSizer = new wxBoxSizer(wxVERTICAL);
    wxBoxSizer* buttonSizer = new wxBoxSizer(wxHORIZONTAL);
    wxButton* addButton = new wxButton(this, ID_AddPhoto, "Add Photo");
    buttonSizer->Add(addButton, 0, wxALL, 10);

    wxButton* backButton = new wxButton(this, ID_BackToMain, "Back");
    buttonSizer->Add(backButton, 0, wxALL, 10);
    mainSizer->Add(buttonSizer, 0, wxALIGN_LEFT);

    photoSizer = new wxGridSizer(0, 3, 10, 10);  
    mainSizer->Add(photoSizer, 1, wxEXPAND | wxALL, 10);

    SetSizer(mainSizer);
    Layout();
    UpdatePhotoDisplay();
}

void AlbumFrame::OnAddPhoto(wxCommandEvent& event)
{
    wxFileDialog openFileDialog(this, _("Open Image file"), "", "",
        "Image files (*.png;*.jpg;*.jpeg)|*.png;*.jpg;*.jpeg", wxFD_OPEN | wxFD_FILE_MUST_EXIST);

    if (openFileDialog.ShowModal() == wxID_CANCEL)
        return; 

    wxString path = openFileDialog.GetPath();

    wxBitmap photo;
    photo.LoadFile(path, wxBITMAP_TYPE_ANY);

    if (photo.IsOk())
    {
        albumPhotos.push_back(photo);
        albumPhotoPaths.push_back(path);  
        wxMessageBox("Photo uploaded successfully!\nFile path: " + path, "Upload Confirmation", wxOK | wxICON_INFORMATION);

        UpdatePhotoDisplay();
        parentFrame->SaveAlbumData(); 
    }
    else
    {
        wxMessageBox("Failed to load the selected photo.", "Error", wxOK | wxICON_ERROR);
    }
}

void AlbumFrame::OnBackToMain(wxCommandEvent& event)
{
    parentFrame->Show();  
    this->Destroy();  
}

void AlbumFrame::UpdatePhotoDisplay()
{
    for (size_t i = 0; i < photoWidgets.size(); ++i)
    {
        photoSizer->Detach(photoWidgets[i]);
        photoWidgets[i]->Destroy();
    }
    photoWidgets.clear();

    for (size_t i = 0; i < albumPhotos.size(); ++i)
    {
        wxStaticBitmap* photoWidget = new wxStaticBitmap(this, wxID_ANY, albumPhotos[i]);
        photoWidget->Bind(wxEVT_LEFT_DOWN, &AlbumFrame::OnPhotoClick, this);  
        photoSizer->Add(photoWidget, 0, wxEXPAND);
        photoWidgets.push_back(photoWidget);
    }

    Layout();
}

void AlbumFrame::OnPhotoClick(wxMouseEvent& event)
{
    wxStaticBitmap* photo = static_cast<wxStaticBitmap*>(event.GetEventObject());
    wxBitmap bitmap = photo->GetBitmap();

    PhotoEditorFrame* editorFrame = new PhotoEditorFrame(bitmap);
    editorFrame->Show();
}


CreateAlbumDialog::CreateAlbumDialog(wxWindow* parent)
    : wxDialog(parent, wxID_ANY, "New Album", wxDefaultPosition, wxSize(300, 300))
{
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);

    wxStaticText* titleText = new wxStaticText(this, wxID_ANY, "Title");
    sizer->Add(titleText, 0, wxALL, 5);

    albumTitle = new wxTextCtrl(this, wxID_ANY);
    sizer->Add(albumTitle, 0, wxEXPAND | wxALL, 5);

    uploadButton = new wxButton(this, ID_UploadPhoto, "Upload Photo");
    sizer->Add(uploadButton, 0, wxALIGN_CENTER | wxALL, 10);

    photoPreview = new wxStaticBitmap(this, wxID_ANY, wxBitmap(), wxDefaultPosition, wxSize(100, 100));
    sizer->Add(photoPreview, 0, wxALIGN_CENTER | wxALL, 10);

    uploadButton->Bind(wxEVT_BUTTON, &CreateAlbumDialog::OnUploadPhoto, this);

    wxButton* okButton = new wxButton(this, wxID_OK, "OK");
    okButton->SetDefault();
    wxButton* cancelButton = new wxButton(this, wxID_CANCEL, "Cancel");

    wxBoxSizer* buttonSizer = new wxBoxSizer(wxHORIZONTAL);
    buttonSizer->Add(okButton, 0, wxALL, 5);
    buttonSizer->Add(cancelButton, 0, wxALL, 5);

    sizer->Add(buttonSizer, 0, wxALIGN_CENTER);

    SetSizerAndFit(sizer);
}

void CreateAlbumDialog::OnUploadPhoto(wxCommandEvent& event)
{
    wxFileDialog openFileDialog(this, _("Open Image file"), "", "",
        "Image files (*.png;*.jpg;*.jpeg)|*.png;*.jpg;*.jpeg", wxFD_OPEN | wxFD_FILE_MUST_EXIST);

    if (openFileDialog.ShowModal() == wxID_CANCEL)
        return; 

    wxString path = openFileDialog.GetPath();
    coverPhoto.LoadFile(path, wxBITMAP_TYPE_ANY);

    if (!coverPhoto.IsOk())
    {
        wxMessageBox("Failed to load photo.", "Error", wxOK | wxICON_ERROR);
        return;
    }

    photoPreview->SetBitmap(wxBitmap(coverPhoto));
    Layout();
}


PhotoEditorFrame::PhotoEditorFrame(wxBitmap bitmap)
    : wxFrame(NULL, wxID_ANY, "Photo Editor", wxDefaultPosition, wxSize(800, 600)),
      originalImage(bitmap.ConvertToImage())
{
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);

    photoDisplay = new wxStaticBitmap(this, wxID_ANY, bitmap);
    sizer->Add(photoDisplay, 1, wxEXPAND | wxALL, 10);

    brightnessSlider = new wxSlider(this, ID_BrightnessSlider, 0, -100, 100, wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL | wxSL_LABELS);
    sizer->Add(new wxStaticText(this, wxID_ANY, "Brightness"), 0, wxALL, 5);
    sizer->Add(brightnessSlider, 0, wxEXPAND | wxALL, 10);

    saturationSlider = new wxSlider(this, ID_SaturationSlider, 0, -100, 100, wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL | wxSL_LABELS);
    sizer->Add(new wxStaticText(this, wxID_ANY, "Saturation"), 0, wxALL, 5);
    sizer->Add(saturationSlider, 0, wxEXPAND | wxALL, 10);

    contrastSlider = new wxSlider(this, ID_ContrastSlider, 0, -100, 100, wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL | wxSL_LABELS);
    sizer->Add(new wxStaticText(this, wxID_ANY, "Contrast"), 0, wxALL, 5);
    sizer->Add(contrastSlider, 0, wxEXPAND | wxALL, 10);

    SetSizer(sizer);
    Layout();
}

void PhotoEditorFrame::OnBrightnessChange(wxCommandEvent& event)
{
    int brightness = brightnessSlider->GetValue();
    wxImage adjustedImage = AdjustBrightness(originalImage, brightness);
    wxBitmap adjustedBitmap(adjustedImage);
    photoDisplay->SetBitmap(adjustedBitmap);
    Layout();
}

void PhotoEditorFrame::OnSaturationChange(wxCommandEvent& event)
{
    int saturation = saturationSlider->GetValue();
    wxImage adjustedImage = AdjustSaturation(originalImage, saturation);
    wxBitmap adjustedBitmap(adjustedImage);
    photoDisplay->SetBitmap(adjustedBitmap);
    Layout();
}

void PhotoEditorFrame::OnContrastChange(wxCommandEvent& event)
{
    int contrast = contrastSlider->GetValue();
    wxImage adjustedImage = AdjustContrast(originalImage, contrast);
    wxBitmap adjustedBitmap(adjustedImage);
    photoDisplay->SetBitmap(adjustedBitmap);
    Layout();
}

wxImage PhotoEditorFrame::AdjustBrightness(wxImage image, int value)
{
    unsigned char* data = image.GetData();
    int length = image.GetWidth() * image.GetHeight() * 3;

    for (int i = 0; i < length; i++) {
        int adjusted = data[i] + value;
        data[i] = std::min(std::max(adjusted, 0), 255);
    }

    return image;
}

wxImage PhotoEditorFrame::AdjustSaturation(wxImage image, int value)
{
    unsigned char* data = image.GetData();
    int length = image.GetWidth() * image.GetHeight() * 3;
    double factor = (value + 100.0) / 100.0;

    for (int i = 0; i < length; i += 3) {
        double r = data[i];
        double g = data[i + 1];
        double b = data[i + 2];

        double gray = 0.3 * r + 0.59 * g + 0.11 * b;

        data[i] = std::min(std::max(int(gray + factor * (r - gray)), 0), 255);
        data[i + 1] = std::min(std::max(int(gray + factor * (g - gray)), 0), 255);
        data[i + 2] = std::min(std::max(int(gray + factor * (b - gray)), 0), 255);
    }

    return image;
}

wxImage PhotoEditorFrame::AdjustContrast(wxImage image, int value)
{
    unsigned char* data = image.GetData();
    int length = image.GetWidth() * image.GetHeight() * 3;
    double contrast = (value + 100.0) / 100.0;

    for (int i = 0; i < length; i++) {
        int newValue = (data[i] - 128) * contrast + 128;
        data[i] = std::min(std::max(newValue, 0), 255);
    }

    return image;
}

void MyFrame::SaveAlbumData()
{
    std::ofstream file(DATA_FILE.mb_str());
    if (!file.is_open()) {
        wxMessageBox("Failed to save album data.", "Error", wxOK | wxICON_ERROR);
        return;
    }

    for (size_t i = 0; i < albumTitles.size(); ++i) {
        file << albumTitles[i].ToStdString() << "\n";
        for (size_t j = 0; j < albumPaths[i].size(); ++j) {
            file << albumPaths[i][j].ToStdString() << "\n";
        }
        file << "END_ALBUM\n";
    }
}

void MyFrame::LoadAlbumData()
{
    std::ifstream file(DATA_FILE.mb_str());
    if (!file.is_open()) {
        return;
    }

    std::string line;
    std::vector<wxBitmap> loadedAlbum;
    std::vector<wxString> loadedPaths;
    wxString currentAlbumTitle;

    while (std::getline(file, line)) {
        if (line == "END_ALBUM") {
            if (!loadedAlbum.empty() || !loadedPaths.empty()) {
                albums.push_back(loadedAlbum);
                albumPaths.push_back(loadedPaths);
                albumTitles.push_back(currentAlbumTitle);
                loadedAlbum.clear();
                loadedPaths.clear();
            }
        } else if (loadedAlbum.empty() && loadedPaths.empty()) {
            currentAlbumTitle = wxString::FromUTF8(line.c_str());
        } else {
            wxString path(line.c_str(), wxConvUTF8);
            wxBitmap bitmap;
            if (bitmap.LoadFile(path, wxBITMAP_TYPE_ANY) && bitmap.IsOk()) {
                loadedAlbum.push_back(bitmap);
                loadedPaths.push_back(path);
            }
        }
    }

    UpdateAlbumDisplay();
}
