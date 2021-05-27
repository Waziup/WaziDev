pipeline {
  agent any
  environment {
    ARDUINO_DIRECTORIES_USER = "$WORKSPACE"
  }
  stages {
    stage('Prepare') {
      steps {
        withEnv(["PATH=$PATH:/home/linuxbrew/.linuxbrew/bin/"]) {
          sh 'arduino-cli core update-index'
          sh 'arduino-cli core install arduino:avr'
          sh './make_all.sh'
        }
      }
    }
  }
  post {
    always {
      junit 'results/TEST-make_all.xml'
    }
    success {
      echo 'Success!'
    }
    failure {
      echo 'Failure!'
    }
    unstable {
      echo 'Unstable'
    }
  }
}
