pipeline {
  agent { dockerfile true } 
  environment {
    USER_LIB_PATH="${WORKSPACE}/libraries"
  }
  stages {
    stage('Build examples') {
      steps {
        sh 'ls'
        sh './make_all.sh'
      }
    }
  }
  post {
    always {
      junit 'tests/report.xml'
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
