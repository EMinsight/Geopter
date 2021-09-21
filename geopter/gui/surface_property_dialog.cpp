#include "surface_property_dialog.h"
#include "ui_surface_property_dialog.h"

SurfacePropertyDialog::SurfacePropertyDialog(std::shared_ptr<OpticalSystem> opt_sys, int surface_index, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SurfacePropertyDialog),
    opt_sys_(opt_sys),
    surface_index_(surface_index)
{
    ui->setupUi(this);

    this->setWindowTitle("Surface " + QString::number(surface_index_));

    QObject::connect(ui->surfaceProfileTypeCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(showSurfaceProfileStackPage(int)));
    ui->sphericalRadiusEdit->setValidator(new QDoubleValidator(-10000.0, 10000.0, 8, this));

    QObject::connect(ui->apertureTypeCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(showApertureStackPage(int)));
    ui->circularRadiusEdit->setValidator(new QDoubleValidator(0.0, 10000.0, 8, this));

    QObject::connect(ui->buttonBox, SIGNAL(accepted()), this, SLOT(onAccept()));

    initializeEvenAsphereDataTable();

    syncUiWithSystem();
}

SurfacePropertyDialog::~SurfacePropertyDialog()
{
    opt_sys_.reset();
    delete ui;
}

void SurfacePropertyDialog::showSurfaceProfileStackPage(int surfaceProfileType)
{
    ui->surfaceProfileStack->setCurrentIndex(surfaceProfileType);
    ui->surfaceProfileStack->show();
}

void SurfacePropertyDialog::showApertureStackPage(int apertureType)
{
    ui->apertureStack->setCurrentIndex(apertureType);
    ui->apertureStack->show();
}

void SurfacePropertyDialog::onAccept()
{
    syncSystemWithUi();
    accept();
}

void SurfacePropertyDialog::syncSystemWithUi()
{
    //-----> surface profile
    int surface_type = ui->surfaceProfileTypeCombo->currentIndex();

    if( surface_type == 0 ){ // Sphere
        double r = ui->sphericalRadiusEdit->text().toDouble();
        double cv = 1.0/r;
        opt_sys_->optical_assembly()->surface(surface_index_)->set_profile(std::make_unique<Spherical>(cv));
    }
    else if(surface_type == 1){ // Even Asphere
        double r = ui->evenAsphereDataTable->item(0,0)->text().toDouble();
        double cv = 1.0/r;
        double k = ui->evenAsphereDataTable->item(1,0)->text().toDouble();

        std::vector<double> coefs(10, 0.0);
        for(int i = 0; i < 10; i++){
            coefs[i] = ui->evenAsphereDataTable->item(i+2, 0)->text().toDouble();
        }

        opt_sys_->optical_assembly()->surface(surface_index_)->set_profile(std::make_unique<EvenPolynomial>(cv,k,coefs));
    }

    //-----> aperture
    int aperture_type = ui->apertureTypeCombo->currentIndex();

    if(aperture_type == 0){ //None
        opt_sys_->optical_assembly()->surface(surface_index_)->remove_clear_aperture();
    }
    else if(aperture_type == 1){ // Circular
        double ap_radius = ui->circularRadiusEdit->text().toDouble();
        opt_sys_->optical_assembly()->surface(surface_index_)->set_clear_aperture(std::make_unique<Circular>(ap_radius));
    }

    // decenter
    // not implemented
}


void SurfacePropertyDialog::syncUiWithSystem()
{
    //==================================
    // surface profile
    std::string surface_type_name = opt_sys_->optical_assembly()->surface(surface_index_)->profile()->name();

    if(surface_type_name == "SPH"){
        ui->surfaceProfileTypeCombo->setCurrentIndex(0);
        ui->surfaceProfileStack->setCurrentIndex(0);
        ui->surfaceProfileStack->show();

        double r = opt_sys_->optical_assembly()->surface(surface_index_)->profile()->radius();
        ui->sphericalRadiusEdit->setText(QString::number(r));
        setValueToCell(ui->evenAsphereDataTable, 0, 0, r);

    }else if(surface_type_name == "ASP"){
        ui->surfaceProfileTypeCombo->setCurrentIndex(1);
        ui->surfaceProfileStack->setCurrentIndex(1);
        ui->surfaceProfileStack->show();
        initializeEvenAsphereDataTable();

        double r = opt_sys_->optical_assembly()->surface(surface_index_)->profile()->radius();
        double k = dynamic_cast<EvenPolynomial*>(opt_sys_->optical_assembly()->surface(surface_index_)->profile())->conic();
        setValueToCell(ui->evenAsphereDataTable, 0, 0, r);
        setValueToCell(ui->evenAsphereDataTable, 1, 0, k);

        for(int i = 0; i < 10; i++){
            double coef = dynamic_cast<EvenPolynomial*>(opt_sys_->optical_assembly()->surface(surface_index_)->profile())->coef(i);
            setValueToCell(ui->evenAsphereDataTable, i+2, 0, coef);
        }
        ui->sphericalRadiusEdit->setText(QString::number(r));
    }


    //=====================================================
    // aperture
    if(opt_sys_->optical_assembly()->surface(surface_index_)->clear_aperture()){
        ui->apertureTypeCombo->setCurrentIndex(1);
        double ap_radius = opt_sys_->optical_assembly()->surface(surface_index_)->clear_aperture()->max_dimension();
        ui->circularRadiusEdit->setText(QString::number(ap_radius));

    }else{ // None
        ui->apertureTypeCombo->setCurrentIndex(0);
    }

}


void SurfacePropertyDialog::initializeEvenAsphereDataTable()
{
    QStringList hLabels = {"Value"};
    ui->evenAsphereDataTable->setColumnCount(hLabels.size());
    ui->evenAsphereDataTable->setHorizontalHeaderLabels(hLabels);

    QStringList vLabels;
    vLabels << "R" << "k";
    for(int i = 1; i <= 10; i++){
        vLabels << ("A" + QString::number((i+1)*2) );
    }
    ui->evenAsphereDataTable->setRowCount(vLabels.size()); // R + k + coef10
    ui->evenAsphereDataTable->setVerticalHeaderLabels(vLabels);

    /*
    // set items
    for(int i = 0; i < ui->evenAsphereDataTable->rowCount(); i++){
        QTableWidgetItem *item = new QTableWidgetItem;
        ui->evenAsphereDataTable->setItem(i,0,item);
    }
    */

}

void SurfacePropertyDialog::setValueToCell(QTableWidget* table, int row, int col, double val)
{
    QTableWidgetItem *item = table->item(row, col);
    if(!item){
        item = new QTableWidgetItem;
        table->setItem(row, col, item);
    }

    item->setTextAlignment(Qt::AlignRight);
    item->setText(QString::number(val, 'g', 8));
}
